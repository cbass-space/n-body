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
        INTEGRATOR_RK4,
    } integrator;
    f32 gravity;
    f32 softening;
    f32 density;
    // bool collide;
    bool paused;
} SimulationOptions;

typedef struct Simulation {
    SimulationOptions options;
    SDL_GPUComputePipeline *euler_pipeline;
    SDL_GPUComputePipeline *verlet_pipeline;
    SDL_GPUComputePipeline *rk4_pipeline;

    GPUArray positions;
    GPUArray velocities;
    GPUArray masses;
    GPUArray movable;
    u32 body_count;
} Simulation;

i32 simulation_init(Simulation *sim, SDL_GPUDevice *gpu);
typedef struct {
    HMM_Vec2 position;
    HMM_Vec2 velocity;
    f32 mass;
    bool movable;
} SimulationAddBodyInfo;
usize simulation_add_body(Simulation *sim, SDL_GPUDevice *gpu, const SimulationAddBodyInfo *body);
void simulation_update(const Simulation *sim, SDL_GPUDevice *gpu, const f64 delta_time);
void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu);

#endif
