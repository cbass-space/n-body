#ifndef N_BODY_CAMERA
#define N_BODY_CAMERA

#include "SDL3/SDL.h"
#include "HandmadeMath.h"
#include "types.h"

typedef struct Simulation Simulation;

typedef struct Camera {
    HMM_Vec2 position;
    usize target;
    f32 zoom;
} Camera;

void camera_init(Camera *cam);
void camera_mouse(Camera *cam, const SDL_Event *event, SDL_Window *window, bool ghost_enabled);
void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim);
void camera_update(Camera *cam, const Simulation *sim);
HMM_Vec2 screen_to_world(const Camera *cam, SDL_Window *window, HMM_Vec2 position);
HMM_Vec2 world_to_screen(const Camera *cam, SDL_Window *window, HMM_Vec2 position);
HMM_Vec2 mouse_world_position(const Camera *cam, SDL_Window *window);

#endif
