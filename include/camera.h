#ifndef N_BODY_CAMERA
#define N_BODY_CAMERA

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_events.h"
#include "HandmadeMath.h"
#include "types.h"

typedef struct Simulation Simulation;

typedef struct Camera {
    HMM_Mat4 orthographic;
    HMM_Mat4 view;
    HMM_Vec2 position;
    HMM_Vec2 window_size;
    u32 target;
    f32 zoom;
} Camera;

void camera_init(Camera *cam);
void camera_update(Camera *cam, SDL_Window *window, SDL_GPUDevice *gpu, const Simulation *sim);
void camera_mouse(Camera *cam, const SDL_Event *event);
void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim);

#endif

