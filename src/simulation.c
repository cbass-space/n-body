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

    sim->euler_pipeline = CreateGPUComputePipeline(gpu, "shaders/simulation/euler.comp.spv");
    sim->verlet_pipeline = CreateGPUComputePipeline(gpu, "shaders/simulation/verlet.comp.spv");
    sim->rk4_pipeline = CreateGPUComputePipeline(gpu, "shaders/simulation/rk4.comp.spv"); // TODO: change to rk4
    if (!sim->euler_pipeline) panic("Failed to create euler compute pipeline!");
    if (!sim->verlet_pipeline) panic("Failed to create verlet compute pipeline!");
    if (!sim->rk4_pipeline) panic("Failed to create rk4 compute pipeline!");

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

u32 simulation_add_body(Simulation *sim, SDL_GPUDevice *gpu, const SimulationAddBodyInfo *body) {
    const AppendGPUArrayBinding bindings[] = {
        { .array = &sim->positions, .source = (u8 *) &body->position, .size = sizeof(HMM_Vec2) },
        { .array = &sim->velocities, .source = (u8 *) &body->velocity, .size = sizeof(HMM_Vec2) },
        { .array = &sim->masses, .source = (u8 *) &body->mass, .size = sizeof(f32) },
        { .array = &sim->movable, .source = (u8 *) &(f32) { body->movable }, .size = sizeof(f32) },
    };

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    AppendGPUArrays(gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return sim->body_count++;
}

void simulation_update(const Simulation *sim, SDL_GPUDevice *gpu, const f32 delta_time) {
    if (sim->options.paused) return;

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);

    const struct {
        u32 body_count;
        f32 gravity;
        f32 softening;
        f32 delta_time;
    } constants = {
        sim->body_count,
        sim->options.gravity,
        sim->options.softening,
        delta_time
    };

    SDL_PushGPUComputeUniformData(command_buffer, 0, &constants, sizeof(constants));

    const SDL_GPUStorageBufferReadWriteBinding buffer_bindings[] = {
        { .buffer = sim->positions.buffer, .cycle = false },
        { .buffer = sim->velocities.buffer, .cycle = false },
    };

    SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(
        command_buffer,
        NULL, 0,
        buffer_bindings, sizeof(buffer_bindings) / sizeof(SDL_GPUStorageBufferReadWriteBinding)
    );

    // TODO: i don't think putting these all into one shader would too bad
    SDL_GPUComputePipeline *integrator_pipeline = NULL;
    switch (sim->options.integrator) {
        case INTEGRATOR_EULER:
            integrator_pipeline = sim->euler_pipeline;
            break;
        case INTEGRATOR_VERLET:
            integrator_pipeline = sim->verlet_pipeline;
            break;
        case INTEGRATOR_RK4:
            integrator_pipeline = sim->rk4_pipeline;
            break;
    }

    SDL_BindGPUComputePipeline(compute_pass, integrator_pipeline);
    SDL_GPUBuffer *buffers[] = { sim->positions.buffer, sim->velocities.buffer, sim->masses.buffer, sim->movable.buffer, };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);

    SDL_EndGPUComputePass(compute_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, sim->euler_pipeline);
    SDL_ReleaseGPUBuffer(gpu, sim->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->movable.buffer);
}
