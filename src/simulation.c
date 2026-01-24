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
        .collide = false,
        .paused = false
    };

    sim->pipeline = CreateGPUComputePipeline(gpu, "shaders/simulation.comp.spv");
    if (!sim->pipeline) return panic("CreateGPUComputePipeline() in simulation_init()", "Failed to create simulation compute pipeline!");

    sim->positions = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);
    sim->velocities = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);
    sim->masses = CreateGPUArray(gpu, sizeof(f32), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
    sim->movable = CreateGPUArray(gpu, sizeof(f32), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
    if (!sim->positions.buffer) return panic("CreateGPUArray() in simulation_init()", "Failed to create simulation position buffer!");
    if (!sim->velocities.buffer) return panic("CreateGPUArray() in simulation_init()", "Failed to create simulation velocities buffer!");
    if (!sim->masses.buffer) return panic("CreateGPUArray() in simulation_init()", "Failed to create simulation masses buffer!");
    if (!sim->movable.buffer) return panic("CreateGPUArray() in simulation_init()", "Failed to create simulation movable buffer!");

    return SDL_APP_CONTINUE;
}

usize simulation_add_body(Simulation *sim, SDL_GPUDevice *gpu, const SimulationAddBodyInfo *body) {
    const AppendGPUArrayBinding bindings[] = {
        { .array = &sim->positions, .source = (u8 *) &body->position, .size = sizeof(HMM_Vec2) },
        { .array = &sim->velocities, .source = (u8 *) &body->velocity, .size = sizeof(HMM_Vec2) },
        { .array = &sim->masses, .source = (u8 *) &body->mass, .size = sizeof(HMM_Vec2) },
        { .array = &sim->movable, .source = (u8 *) &body->movable, .size = sizeof(HMM_Vec2) },
    };

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    AppendGPUArrays(gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return sim->body_count++;
}

void simulation_update(const Simulation *sim, SDL_GPUDevice *gpu, const f64 delta_time) {
    if (sim->options.paused) return;

    SDL_GPUBuffer *buffers[] = {
        sim->positions.buffer,
        sim->velocities.buffer,
        sim->masses.buffer,
        sim->movable.buffer,
    };

    const SDL_GPUStorageBufferReadWriteBinding buffer_bindings[] = {
        { .buffer = sim->positions.buffer, .cycle = false },
        { .buffer = sim->velocities.buffer, .cycle = false },
    };

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_PushGPUComputeUniformData(command_buffer, 0, &sim->options, sizeof(SimulationOptions));
    SDL_PushGPUComputeUniformData(command_buffer, 1, &delta_time, sizeof(f32));

    SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(
        command_buffer,
        NULL, 0,
        buffer_bindings, sizeof(buffer_bindings) / sizeof(SDL_GPUStorageBufferReadWriteBinding)
    );

    SDL_BindGPUComputePipeline(compute_pass, sim->pipeline);
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
    SDL_EndGPUComputePass(compute_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

f32 body_radius(const Simulation *sim, const f32 mass) {
    return powf(mass / sim->options.density, 1.0f/3.0f);
}

void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, sim->pipeline);
    SDL_ReleaseGPUBuffer(gpu, sim->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->movable.buffer);
}
