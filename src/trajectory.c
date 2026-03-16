#include "trajectory.h"
#include "constants.h"
#include "simulation.h"

#include "HandmadeMath.h"

#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTION_LENGTH

SDL_AppResult trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu) {
    trajectories->pipeline = CreateGPUComputePipeline(gpu, "shaders/trajectory.comp.spv");
    if (!trajectories->pipeline) panic("Failed to create trajectories pipeline!");

    trajectories->positions = CreateGPUArray(gpu, PREDICTION_SIZE, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    trajectories->velocities = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    if (!trajectories->positions.buffer) panic("Failed to create trajectory buffer!");
    if (!trajectories->velocities.buffer) panic("Failed to create trajectory buffer!");

    trajectories->enabled = true;
    return SDL_APP_CONTINUE;
}

u32 trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const HMM_Vec2 position) {
    HMM_Vec2 trajectory[PREDICTION_LENGTH];
    for (usize i = 0; i < PREDICTION_LENGTH; i++) trajectory[i] = position;

    const AppendGPUArrayBinding bindings[] = {
        { .array = &trajectories->positions, .source = (u8 *) &trajectory, .size = PREDICTION_SIZE },
        { .array = &trajectories->velocities, .source = (u8 *) &trajectory, .size = sizeof(HMM_Vec2) },
    };

    AppendGPUArrays(gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));
    return trajectories->body_count++;
}

void trajectories_update(const Trajectories *trajectories, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass, const Simulation *sim, const f32 delta_time) {
    if (!trajectories->enabled) return;
    const struct {
        u32 count;
        u32 integrator;
        f32 gravity;
        f32 softening;
        f32 delta_time;
    } constants = {
        sim->body_count,
        sim->options.integrator,
        sim->options.gravity,
        sim->options.softening,
        delta_time,
    };

    SDL_PushGPUComputeUniformData(command_buffer, 0, &constants, sizeof(constants));

    SDL_GPUBuffer *buffers[] = {
        trajectories->positions.buffer,
        trajectories->velocities.buffer,
        sim->positions.buffer,
        sim->velocities.buffer,
        sim->masses.buffer,
        sim->movable.buffer
    };

    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_BindGPUComputePipeline(compute_pass, trajectories->pipeline);

    for (u32 i = 0; i < PREDICTION_LENGTH; i++) {
        SDL_PushGPUComputeUniformData(command_buffer, 1, &i, sizeof(i));
        SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
    }
}

void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, trajectories->pipeline);
    SDL_ReleaseGPUBuffer(gpu, trajectories->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->velocities.buffer);
}
