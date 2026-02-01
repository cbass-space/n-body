#ifndef N_BODY_TRAILS
#define N_BODY_TRAILS

#include "sdl_utils.h"
#include "HandmadeMath.h"

typedef struct Simulation Simulation;

typedef struct Trails {
    SDL_GPUComputePipeline *pipeline;
    GPUArray array;
    u32 body_count;
    u32 frame;
} Trails;

i32 trails_init(Trails *trails, SDL_GPUDevice *gpu);

u32 trails_add_body(Trails *trails, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, HMM_Vec2 position);
void trails_update(Trails *trails, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass, const Simulation *sim);
void trails_free(const Trails *trails, SDL_GPUDevice *gpu);

#endif
