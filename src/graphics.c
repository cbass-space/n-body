#include "graphics.h"
#include "constants.h"
#include "simulation.h"
#include "trails.h"
#include "trajectory.h"
// #include "camera.h"
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
// static void graphics_simulation_update(const Graphics *gfx, const Simulation *sim, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass);
// static void graphics_predictions_update(const Graphics *gfx, const Predictions *predictions, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, bool ghost_enabled);
//
// static void graphics_uniform_matrices(SDL_Window *window, SDL_GPUCommandBuffer *command_buffer, const Camera *cam, u32 slot);
// static void graphics_uniform_constants(Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const SimulationOptions *sim, const Camera *cam, u32 slot);
static void graphics_simulation_draw(const Graphics *gfx, const Simulation *sim, SDL_GPURenderPass *render_pass);
static void graphics_trails_draw(const Graphics *gfx, const Trails *trails, SDL_GPURenderPass *render_pass);
static void graphics_trajectories_draw(const Graphics *gfx, const Trajectories *trajectories, SDL_GPURenderPass *render_pass);
// static void graphics_ghost_draw(const Graphics *gfx, const Ghost *ghost, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass);
static void graphics_gui_draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *swapchain);

void graphics_draw(const Graphics *gfx, const GraphicsDrawInfo *info) {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(info->gpu);
    SDL_GPUTexture *swapchain;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, info->window, &swapchain, NULL, NULL);
    if (!swapchain) {
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    i32 width, height;
    SDL_GetWindowSize(info->window, &width, &height);
    const HMM_Mat4 orthographic = HMM_Orthographic_LH_ZO(
        (f32) -width / 2.0f,
        (f32) width / 2.0f,
        (f32) -height / 2.0f,
        (f32) height / 2.0f,
        0.0f, 1.0f
    );

    const HMM_Mat4 view = HMM_LookAt_LH(
        (HMM_Vec3) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f },
        (HMM_Vec3) { .X = 0.0f, .Y = 0.0f, .Z = 1.0f },
        (HMM_Vec3) { .X = 0.0f, .Y = 1.0f, .Z = 0.0f }
    );

    const HMM_Mat4 matrices[] = { orthographic, view };
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));

    const struct {
        f32 density;
        f32 movable_outline;
        f32 static_outline;
        u32 frame;
        f32 trail_brightness;
        // u32 camera_target;
    } constants = {
        info->sim->options.density,
        gfx->options.movable_outline,
        gfx->options.static_outline,
        info->trails->frame,
        gfx->options.trail_brightness,
        // camera_target
    };

    SDL_PushGPUVertexUniformData(command_buffer, 1, &constants, sizeof(constants));

    // graphics_uniform_matrices(info->window, command_buffer, info->cam, 0);
    // graphics_uniform_constants(gfx, command_buffer, &info->sim->options, info->cam, 1);

    // SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    // graphics_dirty_update(gfx, info->gpu, copy_pass);
    // graphics_simulation_update(gfx, info->sim, info->gpu, copy_pass);
    // graphics_predictions_update(gfx, info->predictions, info->gpu, copy_pass, info->ghost->enabled);
    // SDL_EndGPUCopyPass(copy_pass);

    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &(SDL_GPUColorTargetInfo) {
        .clear_color = gfx->options.clear_color,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .texture = swapchain
    }, 1, NULL);

    graphics_simulation_draw(gfx, info->sim, render_pass);
    graphics_trails_draw(gfx, info->trails, render_pass);
    graphics_trajectories_draw(gfx, info->trajectories, render_pass);
    // graphics_ghost_draw(gfx, info->ghost, command_buffer, render_pass);
    // graphics_predictions_draw(gfx, render_pass, info->predictions->enabled, info->ghost->enabled);
    SDL_EndGPURenderPass(render_pass);

    graphics_gui_draw(command_buffer, swapchain);
    SDL_SubmitGPUCommandBuffer(command_buffer);
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
//
// static void graphics_uniform_matrices(SDL_Window *window, SDL_GPUCommandBuffer *command_buffer, const Camera *cam, u32 slot) {
//     i32 width, height;
//     SDL_GetWindowSize(window, &width, &height);
//     const HMM_Mat4 orthographic = HMM_Orthographic_LH_ZO(
//         cam->zoom * ((f32) -width / 2.0f),
//         cam->zoom * ((f32) width / 2.0f),
//         cam->zoom * ((f32) -height / 2.0f),
//         cam->zoom * ((f32) height / 2.0f),
//         0.0f, 1.0f
//     );
//
//     const HMM_Mat4 view = HMM_LookAt_LH(
//         (HMM_Vec3) { .X = cam->position.X, .Y = cam->position.Y, .Z = 0.0f },
//         (HMM_Vec3) { .X = cam->position.X, .Y = cam->position.Y, .Z = 1.0f },
//         (HMM_Vec3) { .X = 0.0f, .Y = 1.0f, .Z = 0.0f }
//     );
//
//     const HMM_Mat4 matrices[] = { orthographic, view };
//     SDL_PushGPUVertexUniformData(command_buffer, slot, &matrices, sizeof(matrices));
// }
//
// static void graphics_uniform_constants(Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const SimulationOptions *sim, const Camera *cam, const u32 slot) {
//     if (!sim->paused) gfx->trail_counter = (gfx->trail_counter + 1) % TRAIL_LENGTH;
//     const u32 camera_target = cam->target == (usize) -1 ? (u32) -1 : cam->target;
//
//     const struct {
//         f32 density;
//         f32 movable_outline;
//         f32 static_outline;
//         u32 trail_counter;
//         f32 trail_brightness;
//         u32 camera_target;
//     } constants = {
//         sim->density,
//         gfx->options.movable_outline,
//         gfx->options.static_outline,
//         gfx->trail_counter,
//         gfx->options.trail_brightness,
//         camera_target
//     };
//
//     SDL_PushGPUVertexUniformData(command_buffer, slot, &constants, sizeof(constants));
// }
//
// static void graphics_predictions_update(const Graphics *gfx, const Predictions *predictions, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const bool ghost_enabled) {
//     if (!predictions->enabled) return;
//
//     usize bindings_count = 0;
//     WriteGPUBufferBinding bindings[2];
//     if (arrlen(predictions->positions)) {
//         bindings[bindings_count++] = (WriteGPUBufferBinding) {
//             .buffer = gfx->gpu_predictions.buffer,
//             .source = (u8 *) predictions->positions,
//             .size = PREDICTION_SIZE * arrlenu(gfx->colors),
//         };
//     }
//
//     if (ghost_enabled) {
//         bindings[bindings_count++]  = (WriteGPUBufferBinding) {
//             .buffer = gfx->gpu_ghost_predictions,
//             .source = (u8 *) predictions->ghost_positions,
//             .size = PREDICTION_SIZE
//         };
//     }
//
//     WriteToGPUBuffers(gpu, copy_pass, bindings, bindings_count);
// }

static void graphics_simulation_draw(const Graphics *gfx, const Simulation *sim, SDL_GPURenderPass *render_pass) {
    if (!sim->body_count) return;
    SDL_BindGPUGraphicsPipeline(render_pass, gfx->body_pipeline);
    SDL_GPUBuffer *buffers[] = { sim->positions.buffer, gfx->colors.buffer, sim->masses.buffer, sim->movable.buffer, };
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DrawGPUPrimitives(
        render_pass,
        4, sim->body_count,
        0, 0
    );
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
//
// static void graphics_predictions_draw(const Graphics *gfx, SDL_GPURenderPass *render_pass, const bool prediction_enabled, const bool ghost_enabled) {
//     if (!prediction_enabled) return;
//
//     if (arrlenu(gfx->colors)) {
//         SDL_GPUBuffer *buffers[] = { gfx->gpu_predictions.buffer, gfx->gpu_colors.buffer };
//         SDL_BindGPUVertexStorageBuffers(render_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
//         SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_prediction_pipeline);
//         SDL_DrawGPUPrimitives(
//             render_pass,
//             PREDICTIONS_LENGTH, arrlenu(gfx->colors),
//             0, 0
//         );
//     }
//
//     if (ghost_enabled) {
//         SDL_GPUBuffer *ghost_buffers[] = { gfx->gpu_predictions.buffer, gfx->gpu_ghost_predictions };
//         SDL_BindGPUVertexStorageBuffers(render_pass, 0, ghost_buffers, sizeof(ghost_buffers) / sizeof(SDL_GPUBuffer *));
//         SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_ghost_prediction_pipeline);
//         SDL_DrawGPUPrimitives(
//             render_pass,
//             PREDICTIONS_LENGTH, 1,
//             0, 0
//         );
//     }
// }
//
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
