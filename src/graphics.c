#include "graphics.h"
#include "constants.h"
#include "sdl_utils.h"
#include "simulation.h"
#include "ghost.h"
#include "trails.h"
#include "trajectories.h"
#include "camera.h"

#include "SDL3/SDL_gpu.h"
#include "dcimgui.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#define TRAIL_SIZE sizeof(HMM_Vec2) * TRAIL_LENGTH
#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTIONS_LENGTH

SDL_AppResult graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window) {
    gfx->options = (GraphicsOptions) {
        .clear_color = CLEAR_COLOR_DEFAULT,
        .movable_outline = MOVABLE_OUTLINE_DEFAULT,
        .static_outline = STATIC_OUTLINE_DEFAULT,
        .trail_brightness = TRAIL_FADE_DEFAULT
    };

    gfx->body_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/body.vert.spv",
        .fragment_shader_path = "shaders/graphics/circle.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });

    gfx->trail_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/trail.vert.spv",
        .fragment_shader_path = "shaders/graphics/solid.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });

    gfx->trajectory_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/trajectory.vert.spv",
        .fragment_shader_path = "shaders/graphics/solid.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });

    gfx->ghost_body_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/ghost_body.vert.spv",
        .fragment_shader_path = "shaders/graphics/circle.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP
    });

    gfx->ghost_trajectory_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/ghost_trajectory.vert.spv",
        .fragment_shader_path = "shaders/graphics/solid.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });

    if (!gfx->body_pipeline) panic("Failed to create circle graphics pipeline!");
    if (!gfx->trail_pipeline) panic("Failed to create trail graphics pipeline!");
    if (!gfx->trajectory_pipeline) panic("Failed to create trail graphics pipeline!");
    if (!gfx->ghost_body_pipeline) panic("Failed to create ghost body pipeline!");
    if (!gfx->ghost_trajectory_pipeline) panic("Failed to create ghost trajectory pipeline!");

    gfx->colors = CreateGPUArray(gpu, sizeof(SDL_FColor), SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    if (!gfx->colors.buffer) panic("Failed to create color storage buffer!");

    return SDL_APP_CONTINUE;
}

u32 graphics_add_body(Graphics *gfx, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, SDL_FColor *color) {
    AppendGPUArrays(gpu, copy_pass, &(AppendGPUArrayBinding) {
        .array = &gfx->colors,
        .source = (u8 *) color,
        .size = sizeof(SDL_FColor)
    }, 1);

    return gfx->body_count++;
}

static void graphics_uniform_camera(SDL_GPUCommandBuffer *command_buffer, const Camera *cam, const u32 slot);
static void graphics_uniform_constants(SDL_GPUCommandBuffer *command_buffer, const Graphics *gfx, const SimulationOptions *sim, const Trails *trails, const Camera *cam, const u32 slot);

static void graphics_simulation_draw(const Graphics *gfx, const Simulation *sim, SDL_GPURenderPass *render_pass);
static void graphics_ghost_draw(const Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const Ghost *ghost, const Trajectories *trajectories, SDL_GPURenderPass *render_pass);
static void graphics_trails_draw(const Graphics *gfx, const Trails *trails, SDL_GPURenderPass *render_pass);
static void graphics_trajectories_draw(const Graphics *gfx, const Trajectories *trajectories, SDL_GPURenderPass *render_pass);
static void graphics_gui_draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *swapchain);

void graphics_draw(const Graphics *gfx, const GraphicsDrawInfo *info) {
    SDL_GPUTexture *swapchain;
    SDL_WaitAndAcquireGPUSwapchainTexture(info->command_buffer, info->window, &swapchain, NULL, NULL);
    if (!swapchain) {
        SDL_SubmitGPUCommandBuffer(info->command_buffer);
        return;
    }

    graphics_uniform_camera(info->command_buffer, info->cam, 0);
    graphics_uniform_constants(info->command_buffer, gfx, &info->sim->options, info->trails, info->cam, 1);

    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(info->command_buffer, &(SDL_GPUColorTargetInfo) {
        .clear_color = gfx->options.clear_color,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .texture = swapchain
    }, 1, NULL);

    graphics_simulation_draw(gfx, info->sim, render_pass);
    graphics_ghost_draw(gfx, info->command_buffer, info->ghost, info->trajectories, render_pass);
    graphics_trails_draw(gfx, info->trails, render_pass);
    graphics_trajectories_draw(gfx, info->trajectories, render_pass);
    SDL_EndGPURenderPass(render_pass);

    graphics_gui_draw(info->command_buffer, swapchain);
}

static void graphics_uniform_camera(SDL_GPUCommandBuffer *command_buffer, const Camera *cam, const u32 slot) {
    const HMM_Mat4 matrices[] = { cam->orthographic, cam->view };
    SDL_PushGPUVertexUniformData(command_buffer, slot, &matrices, sizeof(matrices));
}

static void graphics_uniform_constants(SDL_GPUCommandBuffer *command_buffer, const Graphics *gfx, const SimulationOptions *sim, const Trails *trails, const Camera *cam, const u32 slot) {
    const struct {
        f32 density;
        f32 movable_outline;
        f32 static_outline;
        u32 trail_target;
        f32 trail_brightness;
        u32 trail_frame;
    } constants = {
        sim->density,
        gfx->options.movable_outline,
        gfx->options.static_outline,
        cam->target,
        gfx->options.trail_brightness,
        trails->frame,
    };

    SDL_PushGPUVertexUniformData(command_buffer, slot, &constants, sizeof(constants));
}

static void graphics_simulation_draw(const Graphics *gfx, const Simulation *sim, SDL_GPURenderPass *render_pass) {
    if (!sim->body_count) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->body_pipeline);
    SDL_GPUBuffer *buffers[] = { sim->positions.buffer, gfx->colors.buffer, sim->masses.buffer, sim->movable.buffer };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        4, sim->body_count,
        0, 0
    );
}

static void graphics_ghost_draw(const Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const Ghost *ghost, const Trajectories *trajectories, SDL_GPURenderPass *render_pass) {
    if (!ghost->enabled) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->ghost_body_pipeline);

    // TODO: can we create a "body" inside Ghost to avoid copy?
    struct {
        SDL_FColor color;
        HMM_Vec2 position;
        f32 mass;
        f32 movable;
    } ghost_info = {
        ghost->color,
        ghost->position,
        ghost->mass,
        ghost->movable
    };

    SDL_PushGPUVertexUniformData(command_buffer, 2, &ghost_info, sizeof(ghost_info));
    SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

    if (trajectories->enabled) {
        SDL_GPUBuffer *buffers[] = { trajectories->positions.buffer, trajectories->ghost };
        SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
        SDL_BindGPUGraphicsPipeline(render_pass, gfx->ghost_trajectory_pipeline);
        SDL_DrawGPUPrimitives(render_pass, PREDICTION_LENGTH, 1, 0, 0);
    }
}

static void graphics_trails_draw(const Graphics *gfx, const Trails *trails, SDL_GPURenderPass *render_pass) {
    if (!trails->body_count) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->trail_pipeline);
    SDL_GPUBuffer *buffers[] = { trails->array.buffer, gfx->colors.buffer };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        TRAIL_LENGTH, trails->body_count,
        0, 0
    );
}

static void graphics_trajectories_draw(const Graphics *gfx, const Trajectories *trajectories, SDL_GPURenderPass *render_pass) {
    if (!trajectories->enabled) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->trajectory_pipeline);
    SDL_GPUBuffer *buffers[] = { trajectories->positions.buffer, gfx->colors.buffer };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        PREDICTION_LENGTH, trajectories->body_count,
        0, 0
    );
}

static void graphics_gui_draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *swapchain) {
    ImDrawData *draw_data = ImGui_GetDrawData();
    cImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &(SDL_GPUColorTargetInfo) {
        .texture = swapchain,
        .load_op =  SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE
    }, 1, NULL);

    cImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
    ImGui_UpdatePlatformWindows();
    ImGui_RenderPlatformWindowsDefault();
    SDL_EndGPURenderPass(render_pass);
}

void graphics_free(const Graphics *gfx, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->body_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->trail_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->trajectory_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->ghost_body_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->ghost_trajectory_pipeline);
    SDL_ReleaseGPUBuffer(gpu, gfx->colors.buffer);
}

