#include "trajectory.h"
#include "constants.h"
#include "simulation.h"

#include "HandmadeMath.h"

#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTION_LENGTH

i32 trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu) {
    trajectories->pipeline = CreateGPUComputePipeline(gpu, "shaders/trajectory.comp.spv");
    if (!trajectories->pipeline) panic("Failed to create trajectories pipeline!");

    trajectories->positions = CreateGPUArray(gpu, PREDICTION_SIZE, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    trajectories->velocities = CreateGPUArray(gpu, PREDICTION_SIZE, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    if (!trajectories->positions.buffer) panic("Failed to create trajectory buffer!");
    if (!trajectories->velocities.buffer) panic("Failed to create trajectory buffer!");

    return SDL_APP_CONTINUE;
}

u32 trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, const HMM_Vec2 position) {
    HMM_Vec2 trajectory[PREDICTION_LENGTH];
    for (usize i = 0; i < PREDICTION_LENGTH; i++) trajectory[i] = position;

    // TODO: shared command buffer across "add_body" and "update" functions
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    const AppendGPUArrayBinding bindings[] = {
        { .array = &trajectories->positions, .source = (u8 *) &trajectory, .size = PREDICTION_SIZE },
        { .array = &trajectories->velocities, .source = (u8 *) &trajectory, .size = PREDICTION_SIZE },
    };

    AppendGPUArrays(gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    return trajectories->body_count++;
}

void trajectories_update(const Trajectories *trajectories, SDL_GPUDevice *gpu, const Simulation * sim, f32 delta_time) {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);

    const struct {
        u32 count;
        f32 gravity;
        f32 softening;
        f32 delta_time;
    } constants = {
        sim->body_count,
        sim->options.gravity,
        sim->options.softening,
        delta_time,
    };

    SDL_PushGPUComputeUniformData(command_buffer, 0, &constants, sizeof(constants));

    const SDL_GPUStorageBufferReadWriteBinding bindings[] = {
        { .buffer = trajectories->positions.buffer, .cycle = false },
        { .buffer = trajectories->velocities.buffer, .cycle = false },
    };

    for (u32 i = 0; i < PREDICTION_LENGTH; i++) {
        SDL_PushGPUComputeUniformData(command_buffer, 1, &i, sizeof(i));

        SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(
            command_buffer,
            NULL, 0,
            bindings, sizeof(bindings) / sizeof(SDL_GPUStorageBufferReadWriteBinding)
        );

        SDL_BindGPUComputePipeline(compute_pass, trajectories->pipeline);

        SDL_GPUBuffer *buffers[] = {
            trajectories->positions.buffer,
            trajectories->velocities.buffer,
            sim->positions.buffer,
            sim->velocities.buffer,
            sim->masses.buffer,
            sim->movable.buffer
        };

        SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
        SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
        SDL_EndGPUComputePass(compute_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, trajectories->pipeline);
    SDL_ReleaseGPUBuffer(gpu, trajectories->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->velocities.buffer);
}
