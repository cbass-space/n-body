#include "graphics.h"
#include "constants.h"
#include "simulation.h"
#include "trails.h"
#include "trajectory.h"
#include "camera.h"
// #include "ghost.h"

#include "stb_ds.h"
#include "SDL3/SDL_gpu.h"
#include "sdl_utils.h"
#include "dcimgui.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#define TRAIL_SIZE sizeof(HMM_Vec2) * TRAIL_LENGTH
#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTIONS_LENGTH

i32 graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window) {
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
    if (!gfx->body_pipeline) panic("Failed to create circle graphics pipeline!");

    gfx->trail_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/trail.vert.spv",
        .fragment_shader_path = "shaders/graphics/trail.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });
    if (!gfx->trail_pipeline) panic("Failed to create trail graphics pipeline!");

    gfx->trajectory_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/graphics/trajectory.vert.spv",
        .fragment_shader_path = "shaders/graphics/trail.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });
    if (!gfx->trajectory_pipeline) panic("Failed to create trail graphics pipeline!");

    // gfx->gpu_ghost_circle_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
    //     .window = window,
    //     .vertex_shader_path = "shaders/graphics/ghost_body.vert.spv",
    //     .fragment_shader_path = "shaders/graphics/circle.frag.spv",
    //     .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    // });
    // if (!gfx->gpu_ghost_circle_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create ghost circle graphics pipeline!");
    //
    // gfx->gpu_ghost_prediction_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
    //     .window = window,
    //     .vertex_shader_path = "shaders/graphics/ghost_prediction.vert.spv",
    //     .fragment_shader_path = "shaders/graphics/trail.frag.spv",
    //     .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
    // });
    // if (!gfx->gpu_ghost_prediction_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create ghost prediction graphics pipeline!");
    //
    gfx->colors = CreateGPUArray(gpu, sizeof(SDL_FColor), SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    if (!gfx->colors.buffer) panic("Failed to create color storage buffer!");
    //
    // gfx->gpu_ghost_predictions = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo) {
    //     .size = PREDICTION_SIZE,
    //     .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
    // });

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

// static void graphics_dirty_update(Graphics *gfx, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass);
static void graphics_uniform_constants(const Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const SimulationOptions *sim, const Trails *trails, const u32 slot);

static void graphics_simulation_draw(const Graphics *gfx, const Simulation *sim, const Camera *cam, SDL_GPURenderPass *render_pass);
static void graphics_trails_draw(const Graphics *gfx, const Trails *trails, const Camera *cam, SDL_GPURenderPass *render_pass);
static void graphics_trajectories_draw(const Graphics *gfx, const Trajectories *trajectories, const Camera *cam, SDL_GPURenderPass *render_pass);
// static void graphics_ghost_draw(const Graphics *gfx, const Ghost *ghost, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass);
static void graphics_gui_draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *swapchain);

void graphics_draw(const Graphics *gfx, const GraphicsDrawInfo *info) {
    SDL_GPUTexture *swapchain;
    SDL_WaitAndAcquireGPUSwapchainTexture(info->command_buffer, info->window, &swapchain, NULL, NULL);
    if (!swapchain) {
        SDL_SubmitGPUCommandBuffer(info->command_buffer);
        return;
    }

    graphics_uniform_constants(gfx, info->command_buffer, &info->sim->options, info->trails, 0);

    // SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    // graphics_dirty_update(gfx, info->gpu, copy_pass);
    // SDL_EndGPUCopyPass(copy_pass);

    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(info->command_buffer, &(SDL_GPUColorTargetInfo) {
        .clear_color = gfx->options.clear_color,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .texture = swapchain
    }, 1, NULL);

    graphics_simulation_draw(gfx, info->sim, info->cam, render_pass);
    graphics_trails_draw(gfx, info->trails, info->cam, render_pass);
    graphics_trajectories_draw(gfx, info->trajectories, info->cam, render_pass);
    // graphics_ghost_draw(gfx, info->ghost, command_buffer, render_pass);
    SDL_EndGPURenderPass(render_pass);

    graphics_gui_draw(info->command_buffer, swapchain);
}

// static void graphics_dirty_update(Graphics *gfx, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass) {
//     if (gfx->dirty_flag.type == DIRTY_NONE) return;
//
//     WriteGPUBufferBinding binding;
//     switch (gfx->dirty_flag.type) {
//         case DIRTY_MASS:
//             binding = (WriteGPUBufferBinding) {
//                 .buffer = gfx->gpu_masses.buffer,
//                 .source = (u8 *) &gfx->dirty_flag.mass,
//                 .size = sizeof(f32),
//                 .buffer_offset = gfx->dirty_flag.index * sizeof(f32)
//             };
//             break;
//         case DIRTY_COLOR:
//             gfx->colors[gfx->dirty_flag.index] = gfx->dirty_flag.color;
//             binding = (WriteGPUBufferBinding) {
//                 .buffer = gfx->gpu_colors.buffer,
//                 .source = (u8 *) &gfx->dirty_flag.color,
//                 .size = 3 * sizeof(f32),
//                 .buffer_offset = gfx->dirty_flag.index * sizeof(SDL_FColor)
//             };
//             break;
//         case DIRTY_MOVABLE:
//             binding = (WriteGPUBufferBinding) {
//                 .buffer = gfx->gpu_movables.buffer,
//                 .source = (u8 *) &(f32) { gfx->dirty_flag.movable },
//                 .size = sizeof(f32),
//                 .buffer_offset = gfx->dirty_flag.index * sizeof(f32)
//             };
//             break;
//         case DIRTY_NONE:
//             fprintf(stdout, "how did we get here?\n");
//             break;
//     }
//
//     WriteToGPUBuffers(gpu, copy_pass, &binding, 1);
//     gfx->dirty_flag.type = DIRTY_NONE;
// }

static void graphics_uniform_constants(const Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const SimulationOptions *sim, const Trails *trails, const u32 slot) {
    const struct {
        f32 density;
        f32 movable_outline;
        f32 static_outline;
        u32 frame;
        f32 trail_brightness;
        // u32 camera_target;
    } constants = {
        sim->density,
        gfx->options.movable_outline,
        gfx->options.static_outline,
        trails->frame,
        gfx->options.trail_brightness,
        // camera_target
    };

    SDL_PushGPUVertexUniformData(command_buffer, slot, &constants, sizeof(constants));
}

static void graphics_simulation_draw(const Graphics *gfx, const Simulation *sim, const Camera *cam, SDL_GPURenderPass *render_pass) {
    if (!sim->body_count) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->body_pipeline);
    SDL_GPUBuffer *buffers[] = { sim->positions.buffer, gfx->colors.buffer, sim->masses.buffer, sim->movable.buffer, cam->matrices };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        4, sim->body_count,
        0, 0
    );
}

static void graphics_trails_draw(const Graphics *gfx, const Trails *trails, const Camera *cam, SDL_GPURenderPass *render_pass) {
    if (!trails->body_count) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->trail_pipeline);
    SDL_GPUBuffer *buffers[] = { trails->array.buffer, gfx->colors.buffer, cam->matrices };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        TRAIL_LENGTH, trails->body_count,
        0, 0
    );
}

static void graphics_trajectories_draw(const Graphics *gfx, const Trajectories *trajectories, const Camera *cam, SDL_GPURenderPass *render_pass) {
    if (!trajectories->enabled) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->trajectory_pipeline);
    SDL_GPUBuffer *buffers[] = { trajectories->positions.buffer, gfx->colors.buffer, cam->matrices };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        PREDICTION_LENGTH, trajectories->body_count,
        0, 0
    );
}

// static void graphics_ghost_draw(const Graphics *gfx, const Ghost *ghost, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass) {
//     if (!ghost->enabled) return;
//
//     const struct {
//         SDL_FColor color;
//         HMM_Vec2 position;
//         f32 mass;
//         f32 movable;
//     } ghost_buffer = {
//         ghost->color,
//         ghost->position,
//         ghost->mass,
//         (f32) ghost->movable
//     };
//
//     SDL_PushGPUVertexUniformData(command_buffer, 2, &ghost_buffer, sizeof(ghost_buffer));
//
//     SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_ghost_circle_pipeline);
//     SDL_DrawGPUPrimitives(
//         render_pass,
//         4, 1,
//         0, 0
//     );
// }

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
    // SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_ghost_circle_pipeline);
    // SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_ghost_prediction_pipeline);

    SDL_ReleaseGPUBuffer(gpu, gfx->colors.buffer);
    // SDL_ReleaseGPUBuffer(gpu, gfx->gpu_predictions.buffer);
    // SDL_ReleaseGPUBuffer(gpu, gfx->gpu_ghost_predictions);
}
