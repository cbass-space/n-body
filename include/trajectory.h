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
    bool enabled;
} Trajectories;

SDL_AppResult trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu);
u32 trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, HMM_Vec2 position);
typedef struct {
    SDL_GPUCommandBuffer *command_buffer;
    SDL_GPUComputePass *compute_pass;
    const Simulation *sim;
    f32 delta_time;
} TrajectoryUpdateInfo;
void trajectories_update(const Trajectories *trajectories, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass, const Simulation *sim, f32 delta_time);
void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu);

#endif
