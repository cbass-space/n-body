#include "SDL3/SDL_gpu.h"
#include "stb_ds.h"
#include "dcimgui.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#include "main.h"

#define TRAIL_SIZE sizeof(HMM_Vec2) * TRAIL_LENGTH
#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTIONS_LENGTH

i32 graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window) {
    gfx->options = (GraphicsOptions) {
        .clear_color = CLEAR_COLOR_DEFAULT,
        .movable_outline = MOVABLE_OUTLINE_DEFAULT,
        .static_outline = STATIC_OUTLINE_DEFAULT,
        .trail_brightness = TRAIL_FADE_DEFAULT
    };

    gfx->gpu_circle_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/circle.vert.spv",
        .fragment_shader_path = "shaders/circle.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });
    if (!gfx->gpu_circle_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create circle graphics pipeline!");

    gfx->gpu_trail_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/trail.vert.spv",
        .fragment_shader_path = "shaders/trail.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });
    if (!gfx->gpu_trail_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create trail graphics pipeline!");

    gfx->gpu_prediction_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/prediction.vert.spv",
        .fragment_shader_path = "shaders/trail.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });

    gfx->gpu_ghost_circle_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/ghost_circle.vert.spv",
        .fragment_shader_path = "shaders/circle.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });
    if (!gfx->gpu_ghost_circle_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create ghost circle graphics pipeline!");

    gfx->gpu_ghost_prediction_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/ghost_prediction.vert.spv",
        .fragment_shader_path = "shaders/trail.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
    });
    if (!gfx->gpu_ghost_prediction_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create ghost prediction graphics pipeline!");

    gfx->gpu_trails = CreateGPUArray(gpu, TRAIL_SIZE);
    gfx->gpu_predictions = CreateGPUArray(gpu, PREDICTION_SIZE);
    gfx->gpu_masses = CreateGPUArray(gpu, sizeof(f32));
    gfx->gpu_movables = CreateGPUArray(gpu, sizeof(f32));
    gfx->gpu_colors = CreateGPUArray(gpu, sizeof(SDL_FColor));
    if (!gfx->gpu_trails.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create trail storage buffer!");
    if (!gfx->gpu_predictions.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create predictions storage buffer!");
    if (!gfx->gpu_masses.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create mass storage buffer!");
    if (!gfx->gpu_movables.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create movables buffer!");
    if (!gfx->gpu_colors.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create color storage buffer!");

    gfx->gpu_ghost_predictions = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo) {
        .size = PREDICTION_SIZE,
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
    });

    return 0;
}

usize graphics_add_body(Graphics *gfx, GraphicsAddBodyInfo *info) {
    HMM_Vec2 trail_positions[TRAIL_LENGTH];
    HMM_Vec2 prediction_positions[PREDICTIONS_LENGTH];
    for (usize i = 0; i < TRAIL_LENGTH; i++) trail_positions[i] = info->sim->r[info->index];
    for (usize i = 0; i < PREDICTIONS_LENGTH; i++) prediction_positions[i] = info->sim->r[info->index];

    const AppendGPUArrayBinding bindings[] = {
        { .array = &gfx->gpu_trails, .source = (u8*) &trail_positions, .size = TRAIL_SIZE },
        { .array = &gfx->gpu_predictions, .source = (u8*) &prediction_positions, .size = PREDICTION_SIZE },
        { .array = &gfx->gpu_masses, .source = (u8*) &info->sim->m[info->index], .size = sizeof(f32) },
        { .array = &gfx->gpu_movables, .source = (u8*) &(f32) { info->sim->movable[info->index] }, .size = sizeof(f32) },
        { .array = &gfx->gpu_colors, .source = (u8*) &info->color, .size = sizeof(SDL_FColor) },
    };

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(info->gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    AppendGPUArrays(info->gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    const usize index = arrlenu(gfx->colors);
    arrput(gfx->colors, info->color);
    return index;
}

void graphics_draw(Graphics *gfx, const GraphicsDrawInfo *info) {
    // TODO: what does and doesn't need to run when len(gfx->colors) == 0 and !ghost->mode
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(info->gpu);

    SDL_GPUTexture *swapchain;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, info->window, &swapchain, NULL, NULL);
    if (!swapchain) {
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (gfx->dirty_flag.type != DIRTY_NONE) {
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        UploadGPUBufferBinding binding;

        switch (gfx->dirty_flag.type) {
            case DIRTY_MASS:
                binding = (UploadGPUBufferBinding) {
                    .buffer = gfx->gpu_masses.buffer,
                    .source = (u8*) &gfx->dirty_flag.mass,
                    .size = sizeof(f32),
                    .buffer_offset = gfx->dirty_flag.index * sizeof(f32)
                };
                break;
            case DIRTY_COLOR:
                gfx->colors[gfx->dirty_flag.index] = gfx->dirty_flag.color;
                binding = (UploadGPUBufferBinding) {
                    .buffer = gfx->gpu_colors.buffer,
                    .source = (u8*) &gfx->dirty_flag.color,
                    .size = 3 * sizeof(f32),
                    .buffer_offset = gfx->dirty_flag.index * sizeof(SDL_FColor)
                };
                break;
            case DIRTY_MOVABLE:
                binding = (UploadGPUBufferBinding) {
                    .buffer = gfx->gpu_movables.buffer,
                    .source = (u8*) &(f32) { gfx->dirty_flag.movable },
                    .size = sizeof(f32),
                    .buffer_offset = gfx->dirty_flag.index * sizeof(f32)
                };
                break;
            case DIRTY_NONE:
                fprintf(stdout, "how did we get here?\n");
                break;
        }

        UploadIntoGPUBuffers(info->gpu, copy_pass, &binding, 1);
        SDL_EndGPUCopyPass(copy_pass);
        gfx->dirty_flag.type = DIRTY_NONE;
    }

    i32 width, height;
    SDL_GetWindowSize(info->window, &width, &height);
    const HMM_Mat4 orthographic = HMM_Orthographic_LH_ZO(
        info->cam->zoom * ((f32) -width / 2.0f),
        info->cam->zoom * ((f32) width / 2.0f),
        info->cam->zoom * ((f32) -height / 2.0f),
        info->cam->zoom * ((f32) height / 2.0f),
        0.0f, 1.0f
    );

    const HMM_Mat4 view = HMM_LookAt_LH(
        (HMM_Vec3) { .X = info->cam->position.X, .Y = info->cam->position.Y, .Z = 0.0f },
        (HMM_Vec3) { .X = info->cam->position.X, .Y = info->cam->position.Y, .Z = 1.0f },
        (HMM_Vec3) { .X = 0.0f, .Y = 1.0f, .Z = 0.0f }
    );

    gfx->trail_counter = (gfx->trail_counter + 1) % TRAIL_LENGTH;
    const u32 camera_target = info->cam->target == (usize) -1 ? (u32) -1 : info->cam->target;
    const HMM_Mat4 matrices[] = { orthographic, view };
    const struct {
        f32 density;
        f32 movable_outline;
        f32 static_outline;
        u32 trail_counter;
        f32 trail_brightness;
        u32 camera_target;
    } constants = {
        info->sim->options.density,
        gfx->options.movable_outline,
        gfx->options.static_outline,
        gfx->trail_counter,
        gfx->options.trail_brightness,
        camera_target
    };

    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));
    SDL_PushGPUVertexUniformData(command_buffer, 1, &constants, sizeof(constants));

    u32 num_bindings = 0;
    UploadGPUBufferBinding *bindings = SDL_malloc((arrlenu(gfx->colors) + 2) * sizeof(UploadGPUBufferBinding));
    for (usize i = 0; i < arrlenu(gfx->colors); i++) {
        bindings[num_bindings] = (UploadGPUBufferBinding) {
            .buffer = gfx->gpu_trails.buffer,
            .source = (u8*) &info->sim->r[i],
            .size = sizeof(HMM_Vec2),
            .buffer_offset = (i * TRAIL_SIZE + gfx->trail_counter * sizeof(HMM_Vec2)),
        };
        num_bindings++;
    }

    if (arrlen(info->predictions->positions)) {
        bindings[num_bindings] = (UploadGPUBufferBinding) {
            .buffer = gfx->gpu_predictions.buffer,
            .source = (u8*) info->predictions->positions,
            .size = PREDICTION_SIZE * arrlenu(gfx->colors),
        };
        num_bindings++;
    }

    if (info->ghost->mode) {
        bindings[num_bindings]  = (UploadGPUBufferBinding) {
            .buffer = gfx->gpu_ghost_predictions,
            .source = (u8*) info->predictions->ghost_positions,
            .size = PREDICTION_SIZE
        };
        num_bindings++;
    }

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    UploadIntoGPUBuffers(info->gpu, copy_pass, bindings, num_bindings);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_free(bindings);

    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &(SDL_GPUColorTargetInfo) {
        .clear_color = gfx->options.clear_color,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .texture = swapchain
    }, 1, NULL);

    SDL_GPUBuffer *storage_buffers[] = {
        gfx->gpu_trails.buffer,
        gfx->gpu_colors.buffer,
        gfx->gpu_masses.buffer,
        gfx->gpu_movables.buffer,
    };

    SDL_BindGPUVertexStorageBuffers(render_pass, 0, storage_buffers, sizeof(storage_buffers) / sizeof(SDL_GPUBuffer*));

    if (arrlenu(gfx->colors)) {
        SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_circle_pipeline);
        SDL_DrawGPUPrimitives(
            render_pass,
            4, arrlenu(gfx->colors),
            0, 0
        );

        SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_trail_pipeline);
        SDL_DrawGPUPrimitives(
            render_pass,
            TRAIL_LENGTH, arrlenu(gfx->colors),
            0, 0
        );

        SDL_BindGPUVertexStorageBuffers(render_pass, 0, &gfx->gpu_predictions.buffer, 1);
        SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_prediction_pipeline);
        SDL_DrawGPUPrimitives(
            render_pass,
            PREDICTIONS_LENGTH, arrlenu(gfx->colors),
            0, 0
        );
    }

    if (info->ghost->mode) {
        const struct {
            SDL_FColor color;
            HMM_Vec2 position;
            f32 mass;
            f32 movable;
        } ghost = {
            info->ghost->color,
            info->ghost->position,
            info->ghost->mass,
            (f32) info->ghost->movable
        };

        SDL_PushGPUVertexUniformData(command_buffer, 2, &ghost, sizeof(ghost));

        SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_ghost_circle_pipeline);
        SDL_DrawGPUPrimitives(
            render_pass,
            4, 1,
            0, 0
        );

        SDL_GPUBuffer *ghost_buffers[] = { gfx->gpu_predictions.buffer, gfx->gpu_ghost_predictions };
        SDL_BindGPUVertexStorageBuffers(render_pass, 0, ghost_buffers, sizeof(ghost_buffers) / sizeof(SDL_GPUBuffer*));
        SDL_BindGPUGraphicsPipeline(render_pass, gfx->gpu_ghost_prediction_pipeline);
        SDL_DrawGPUPrimitives(
            render_pass,
            PREDICTIONS_LENGTH, 1,
            0, 0
        );
    }

    SDL_EndGPURenderPass(render_pass);

    ImDrawData *draw_data = ImGui_GetDrawData();
    cImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
    render_pass = SDL_BeginGPURenderPass(command_buffer, &(SDL_GPUColorTargetInfo) {
        .texture = swapchain,
        .load_op =  SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE
    }, 1, NULL);

    cImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
    ImGui_UpdatePlatformWindows();
    ImGui_RenderPlatformWindowsDefault();

    SDL_EndGPURenderPass(render_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

void graphics_free(const Graphics *gfx, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_circle_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_trail_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_prediction_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_ghost_circle_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->gpu_ghost_prediction_pipeline);

    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_trails.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_predictions.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_movables.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_colors.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_ghost_predictions);
}
