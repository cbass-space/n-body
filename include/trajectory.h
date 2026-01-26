#ifndef N_BODY_TRAJECTORY
#define N_BODY_TRAJECTORY

#include "HandmadeMath.h"
#include "sdl_utils.h"

typedef struct Simulation Simulation;

typedef struct Trajectories {
    SDL_GPUComputePipeline *pipeline;
    GPUArray positions;
    GPUArray velocities;
    u32 body_count;
} Trajectories;

i32 trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu);
u32 trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, HMM_Vec2 position);
void trajectories_update(const Trajectories *trajectories, SDL_GPUDevice *gpu, const Simulation * sim, f32 delta_time);
void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu);

#endif
