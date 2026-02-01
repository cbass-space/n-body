#include "simulation.h"
#include "constants.h"
#include "sdl_utils.h"

#include "stb_ds.h"
#include "SDL3/SDL.h"

i32 simulation_init(Simulation *sim, SDL_GPUDevice *gpu) {
    sim->options = (SimulationOptions) {
        .gravity = GRAVITY_DEFAULT,
        .softening = SOFTENING_DEFAULT,
        .density = DENSITY_DEFAULT,
        .integrator = INTEGRATOR_DEFAULT,
        .paused = false
    };

    sim->pipeline = CreateGPUComputePipeline(gpu, "shaders/simulation.comp.spv");
    if (!sim->pipeline) panic("Failed to create simulation compute pipeline!");

    sim->positions = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    sim->velocities = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    sim->masses = CreateGPUArray(gpu, sizeof(f32), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    sim->movable = CreateGPUArray(gpu, sizeof(f32), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    if (!sim->positions.buffer) panic("Failed to create simulation position buffer!");
    if (!sim->velocities.buffer) panic("Failed to create simulation velocities buffer!");
    if (!sim->masses.buffer) panic("Failed to create simulation masses buffer!");
    if (!sim->movable.buffer) panic("Failed to create simulation movable buffer!");

    return SDL_APP_CONTINUE;
}

u32 simulation_add_body(Simulation *sim, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const SimulationAddBodyInfo *body) {
    const AppendGPUArrayBinding bindings[] = {
        { .array = &sim->positions, .source = (u8 *) &body->position, .size = sizeof(HMM_Vec2) },
        { .array = &sim->velocities, .source = (u8 *) &body->velocity, .size = sizeof(HMM_Vec2) },
        { .array = &sim->masses, .source = (u8 *) &body->mass, .size = sizeof(f32) },
        { .array = &sim->movable, .source = (u8 *) &(f32) { body->movable }, .size = sizeof(f32) },
    };

    AppendGPUArrays(gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));
    return sim->body_count++;
}

void simulation_update(const Simulation *sim, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass, const f32 delta_time) {
    if (sim->options.paused) return;

    const struct {
        u32 body_count;
        u32 integrator;
        f32 gravity;
        f32 softening;
        f32 delta_time;
    } constants = {
        sim->body_count,
        sim->options.integrator,
        sim->options.gravity,
        sim->options.softening,
        delta_time
    };

    SDL_PushGPUComputeUniformData(command_buffer, 0, &constants, sizeof(constants));

    SDL_BindGPUComputePipeline(compute_pass, sim->pipeline);
    SDL_GPUBuffer *buffers[] = { sim->positions.buffer, sim->velocities.buffer, sim->masses.buffer, sim->movable.buffer, };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
}

void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, sim->pipeline);
    SDL_ReleaseGPUBuffer(gpu, sim->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->movable.buffer);
}
