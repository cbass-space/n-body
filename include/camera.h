#ifndef N_BODY_CAMERA
#define N_BODY_CAMERA

#include "SDL3/SDL.h"
#include "SDL3/SDL_gpu.h"
#include "HandmadeMath.h"
#include "types.h"

typedef struct Simulation Simulation;

typedef struct Camera {
    SDL_GPUComputePipeline *pipeline;
    SDL_GPUBuffer *buffer;
    SDL_GPUBuffer *matrices;
} Camera;

SDL_AppResult camera_init(Camera *cam, SDL_Window *window, SDL_GPUDevice *gpu);
void camera_free(const Camera *cam, SDL_GPUDevice *gpu);
// void camera_mouse(Camera *cam, const SDL_Event *event, SDL_Window *window, bool ghost_enabled);
// void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim);
// void camera_update(Camera *cam, const Simulation *sim);
// HMM_Vec2 screen_to_world(const Camera *cam, SDL_Window *window, HMM_Vec2 position);
// HMM_Vec2 world_to_screen(const Camera *cam, SDL_Window *window, HMM_Vec2 position);
// HMM_Vec2 mouse_world_position(const Camera *cam, SDL_Window *window);

#endif
