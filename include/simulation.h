#ifndef N_BODY_SIMULATION
#define N_BODY_SIMULATION

#include <stdbool.h>
#include "SDL3/SDL_gpu.h"
#include "HandmadeMath.h"
#include "sdl_utils.h"
#include "types.h"

typedef struct {
    enum {
        INTEGRATOR_EULER,
        INTEGRATOR_VERLET,
        INTEGRATOR_RUNGE_KUTTA_4,
    } integrator;
    f32 gravity;
    f32 softening;
    f32 density;
    bool paused;
} SimulationOptions;

typedef struct Simulation {
    SimulationOptions options;
    SDL_GPUComputePipeline *integrators[3];

    GPUArray positions;
    GPUArray velocities;
    GPUArray masses;
    GPUArray movable;
    u32 body_count;
} Simulation;

SDL_AppResult simulation_init(Simulation *sim, SDL_GPUDevice *gpu);
typedef struct {
    HMM_Vec2 position;
    HMM_Vec2 velocity;
    f32 mass;
    bool movable;
} SimulationAddBodyInfo;

u32 simulation_add_body(Simulation *sim, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const SimulationAddBodyInfo *body);
void simulation_update(const Simulation *sim, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass, f32 delta_time);
void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu);

#endif
