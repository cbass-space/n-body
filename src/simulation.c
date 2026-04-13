#include "simulation.h"
#include "constants.h"
#include "sdl_utils.h"

#include "stb_ds.h"

SDL_AppResult simulation_init(Simulation *sim, SDL_GPUDevice *gpu) {
    sim->options = (SimulationOptions) {
        .gravity = GRAVITY_DEFAULT,
        .softening = SOFTENING_DEFAULT,
        .density = DENSITY_DEFAULT,
        .integrator = INTEGRATOR_DEFAULT,
        .paused = false
    };

    SDL_GPUComputePipeline *euler = CreateGPUComputePipeline(gpu, "shaders/simulation/euler.comp.spv");
    SDL_GPUComputePipeline *verlet = CreateGPUComputePipeline(gpu, "shaders/simulation/verlet.comp.spv");
    SDL_GPUComputePipeline *runge_kutta = CreateGPUComputePipeline(gpu, "shaders/simulation/runge_kutta.comp.spv");
    if (!euler) panic("Failed to create simulation euler compute pipeline!");
    if (!verlet) panic("Failed to create simulation verlet compute pipeline!");
    if (!runge_kutta) panic("Failed to create simulation runge kutta compute pipeline!");
    memcpy(sim->integrators, (SDL_GPUComputePipeline*[3]) { euler, verlet, runge_kutta }, sizeof(sim->integrators));

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

    SDL_GPUComputePipeline *integrator = sim->integrators[sim->options.integrator];
    SDL_BindGPUComputePipeline(compute_pass, integrator);

    SDL_GPUBuffer *buffers[] = { sim->positions.buffer, sim->velocities.buffer, sim->masses.buffer, sim->movable.buffer, };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));
    SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
}

void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu) {
    for (u8 i = 0; i < 3; i++) SDL_ReleaseGPUComputePipeline(gpu, sim->integrators[i]);
    SDL_ReleaseGPUBuffer(gpu, sim->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->movable.buffer);
}
