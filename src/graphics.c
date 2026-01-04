#include "SDL3/SDL_gpu.h"
#include "stb_ds.h"

#include "main.h"

#define TRAIL_SIZE sizeof(HMM_Vec2) * TRAIL_LENGTH

i32 graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window) {
    gfx->options = (GraphicsOptions) {
        .clear_color = CLEAR_COLOR_DEFAULT,
        .movable_outline = MOVABLE_OUTLINE_DEFAULT,
        .static_outline = STATIC_OUTLINE_DEFAULT,
        .trail_brightness = TRAIL_FADE_DEFAULT
    };

    gfx->gpu_trails = CreateGPUArray(gpu, TRAIL_SIZE);
    gfx->gpu_offsets = CreateGPUArray(gpu, sizeof(u32));
    gfx->gpu_masses = CreateGPUArray(gpu, sizeof(f32));
    gfx->gpu_movables = CreateGPUArray(gpu, sizeof(f32));
    gfx->gpu_colors = CreateGPUArray(gpu, sizeof(SDL_FColor));
    if (!gfx->gpu_trails.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create trail storage buffer!");
    if (!gfx->gpu_offsets.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create trail offsets buffer!");
    if (!gfx->gpu_masses.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create mass storage buffer!");
    if (!gfx->gpu_movables.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create movables buffer!");
    if (!gfx->gpu_colors.buffer) return panic("CreateGPUArray() in graphics_init()", "Failed to create color storage buffer!");

    gfx->circle_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/circle.vert.spv",
        .fragment_shader_path = "shaders/circle.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });

    if (!gfx->circle_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create circle graphics pipeline!");

    gfx->trail_pipeline = CreateGPUGraphicsPipeline(gpu, &(CreateGPUGraphicsPipelineInfo) {
        .window = window,
        .vertex_shader_path = "shaders/trail.vert.spv",
        .fragment_shader_path = "shaders/trail.frag.spv",
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP
    });

    if (!gfx->trail_pipeline) return panic("SDL_CreateGPUGraphicsPipeline() in graphics_init()", "Failed to create trail graphics pipeline!");

    return 0;
}

usize graphics_push_body(Graphics *gfx, SDL_GPUDevice *gpu, const NewBody *body) {
    HMM_Vec2 trail_positions[TRAIL_LENGTH];
    for (usize i = 0; i < TRAIL_LENGTH; i++) {
        trail_positions[i] = body->position;
    }

    const AppendGPUArrayBinding bindings[] = {
        { .array = &gfx->gpu_trails, .source = (u8*) &trail_positions, .size = TRAIL_SIZE },
        { .array = &gfx->gpu_offsets, .source = (u8*) &gfx->trail_counter, .size = sizeof(u32) },
        { .array = &gfx->gpu_masses, .source = (u8*) &body->mass, .size = sizeof(f32) },
        { .array = &gfx->gpu_movables, .source = (u8*) &(float) { body->movable }, .size = sizeof(f32) },
        { .array = &gfx->gpu_colors, .source = (u8*) &body->color, .size = sizeof(SDL_FColor) },
    };

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    AppendGPUArrays(gpu, copy_pass, bindings, 5);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    const usize index = arrlenu(gfx->offsets);
    arrput(gfx->offsets, gfx->trail_counter);
    arrput(gfx->colors, body->color);
    return index;
}

void graphics_draw(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window, const Simulation *sim, const Camera *cam) {
    if (gfx->dirty_flag.type != DIRTY_NONE) {
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(gfx->command_buffer);
        UploadGPUBufferBinding binding;
        if (gfx->dirty_flag.type == DIRTY_MASS) {
            binding = (UploadGPUBufferBinding) {
                .buffer = gfx->gpu_masses.buffer,
                .source = (u8*) &gfx->dirty_flag.mass,
                .size = sizeof(f32),
                .buffer_offset = gfx->dirty_flag.index * sizeof(f32)
            };
        } else if (gfx->dirty_flag.type == DIRTY_COLOR) {
            gfx->colors[gfx->dirty_flag.index] = gfx->dirty_flag.color;
            binding = (UploadGPUBufferBinding) {
                .buffer = gfx->gpu_colors.buffer,
                .source = (u8*) &gfx->dirty_flag.color,
                .size = 3 * sizeof(f32),
                .buffer_offset = gfx->dirty_flag.index * sizeof(SDL_FColor)
            };
        } else if (gfx->dirty_flag.type == DIRTY_MOVABLE) {
            binding = (UploadGPUBufferBinding) {
                .buffer = gfx->gpu_movables.buffer,
                .source = (u8*) &(float) { gfx->dirty_flag.movable },
                .size = sizeof(f32),
                .buffer_offset = gfx->dirty_flag.index * sizeof(f32)
            };
        }

        UploadIntoGPUBuffers(gpu, copy_pass, &binding, 1);
        SDL_EndGPUCopyPass(copy_pass);
        gfx->dirty_flag.type = DIRTY_NONE;
    }

    i32 width, height;
    SDL_GetWindowSize(window, &width, &height);
    const HMM_Mat4 orthographic = HMM_Orthographic_LH_ZO(
        cam->zoom * ((f32) -width / 2.0f),
        cam->zoom * ((f32) width / 2.0f),
        cam->zoom * ((f32) -height / 2.0f),
        cam->zoom * ((f32) height / 2.0f),
        0.0f, 1.0f
    );

    const HMM_Mat4 view = HMM_LookAt_LH(
        (HMM_Vec3) { .X = cam->position.X, .Y = cam->position.Y, .Z = 0.0f },
        (HMM_Vec3) { .X = cam->position.X, .Y = cam->position.Y, .Z = 1.0f },
        (HMM_Vec3) { .X = 0.0f, .Y = 1.0f, .Z = 0.0f }
    );

    const HMM_Mat4 matrices[] = { orthographic, view };
    const f32 constants[] = { sim->options.density, gfx->options.movable_outline, gfx->options.static_outline, gfx->options.trail_brightness };
    gfx->trail_counter = (gfx->trail_counter + 1) % TRAIL_LENGTH;

    SDL_PushGPUVertexUniformData(gfx->command_buffer, 0, &matrices, sizeof(matrices));
    SDL_PushGPUVertexUniformData(gfx->command_buffer, 1, &gfx->trail_counter, sizeof(u32));
    SDL_PushGPUVertexUniformData(gfx->command_buffer, 2, &constants, sizeof(constants));

    UploadGPUBufferBinding *position_bindings = SDL_malloc(sizeof(UploadGPUBufferBinding) * arrlenu(gfx->offsets));
    for (usize i = 0; i < arrlenu(gfx->offsets); i++) {
        const u32 buffer_offset = (i * TRAIL_SIZE)
            + ((gfx->trail_counter - gfx->offsets[i]) % TRAIL_LENGTH)
            * sizeof(HMM_Vec2);
        position_bindings[i] = (UploadGPUBufferBinding) {
            .buffer = gfx->gpu_trails.buffer,
            .source = (u8*) &sim->r[i],
            .size = sizeof(HMM_Vec2),
            .buffer_offset = buffer_offset,
        };
    }

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(gfx->command_buffer);
    UploadIntoGPUBuffers(gpu, copy_pass, position_bindings, arrlen(gfx->offsets));
    SDL_EndGPUCopyPass(copy_pass);
    SDL_free(position_bindings);

    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(gfx->command_buffer, &(SDL_GPUColorTargetInfo) {
        .clear_color = gfx->options.clear_color,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .texture = gfx->swapchain
    }, 1, NULL);


    if (!arrlenu(gfx->offsets)) {
        SDL_EndGPURenderPass(render_pass);
        return;
    }

    SDL_GPUBuffer *storage_buffers[] = {
        gfx->gpu_trails.buffer,
        gfx->gpu_offsets.buffer,
        gfx->gpu_colors.buffer,
        gfx->gpu_masses.buffer,
        gfx->gpu_movables.buffer,
    };

    SDL_BindGPUGraphicsPipeline(render_pass, gfx->circle_pipeline);
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, storage_buffers, 5);
    SDL_DrawGPUPrimitives(
        render_pass,
        4, arrlenu(gfx->offsets),
        0, 0
    );

    SDL_BindGPUGraphicsPipeline(render_pass, gfx->trail_pipeline);
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, storage_buffers, 3);
    SDL_DrawGPUPrimitives(
        render_pass,
        TRAIL_LENGTH, arrlenu(gfx->offsets),
        0, 0
    );

    SDL_EndGPURenderPass(render_pass);
}

void graphics_free(Graphics *gfx, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_colors.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_trails.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_offsets.buffer);
    SDL_ReleaseGPUBuffer(gpu, gfx->gpu_movables.buffer);
    arrfree(gfx->offsets);

    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->circle_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(gpu, gfx->trail_pipeline);
}
