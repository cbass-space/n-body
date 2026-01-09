#include "main.h"
#include "stb_ds.h"

void camera_init(Camera *cam) {
    cam->zoom = 1.0f;
    cam->target = (usize) -1;
}

HMM_Vec2 screen_to_world(const Camera *cam, SDL_Window *window, HMM_Vec2 position) {
    i32 width, height;
    SDL_GetWindowSize(window, &width, &height);
    const HMM_Vec2 center = (HMM_Vec2) { .X = (f32) width / 2.0f, .Y = (f32) height / 2.0f };
    position.Y = (f32) height - position.Y;

    HMM_Vec2 camera_to_point = HMM_SubV2(position, center);
    camera_to_point = HMM_MulV2F(camera_to_point, cam->zoom);
    return HMM_AddV2(cam->position, camera_to_point);
}

HMM_Vec2 world_to_screen(const Camera *cam, SDL_Window *window, const HMM_Vec2 position) {
    i32 width, height;
    SDL_GetWindowSize(window, &width, &height);
    const HMM_Vec2 center = (HMM_Vec2) { .X = (f32) width / 2.0f, .Y = (f32) height / 2.0f };

    HMM_Vec2 camera_to_point = HMM_SubV2(position, cam->position);
    camera_to_point = HMM_DivV2F(camera_to_point, cam->zoom);
    HMM_Vec2 screen = HMM_AddV2(center, camera_to_point);
    screen.Y = (f32) height - screen.Y;
    return screen;
}

HMM_Vec2 mouse_world_position(const Camera *cam, SDL_Window *window) {
    HMM_Vec2 mouse = { 0 };
    SDL_GetMouseState(&mouse.X, &mouse.Y);
    return screen_to_world(cam, window, mouse);
}

void camera_mouse(Camera *cam, const SDL_Event *event, SDL_Window *window, const bool ghost_mode) {
    HMM_Vec2 mouse_delta = { 0 };
    if (SDL_GetRelativeMouseState(&mouse_delta.X, &mouse_delta.Y) & SDL_BUTTON_RMASK) {
        cam->target = (usize) -1;
        mouse_delta.Y *= -1.0f; // screen to world coordinate system
        mouse_delta = HMM_MulV2F(mouse_delta, -cam->zoom);
        cam->position = HMM_AddV2(cam->position, mouse_delta);
    }

    if (event->type == SDL_EVENT_MOUSE_WHEEL && !ghost_mode) {
        const HMM_Vec2 mouse_old = mouse_world_position(cam, window);
        cam->zoom *= SDL_expf(event->wheel.y * -0.1f);

        if (cam->target == (usize) -1) {
            const HMM_Vec2 mouse_new = mouse_world_position(cam, window);
            const HMM_Vec2 delta = HMM_SubV2(mouse_new, mouse_old);
            cam->position = HMM_SubV2(cam->position, delta);
        }
    }
}

void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim) {
    if (event->type != SDL_EVENT_KEY_DOWN) return;
    if (event->key.scancode == SDL_SCANCODE_RIGHTBRACKET) cam->target = (cam->target + 1) % arrlenu(sim->r);
    if (event->key.scancode == SDL_SCANCODE_LEFTBRACKET) cam->target = (cam->target - 1 + arrlenu(sim->r)) % arrlenu(sim->r);
}

void camera_update(Camera *cam, const Simulation *sim) {
    if (cam->target == (usize) -1) return;
    cam->position = sim->r[cam->target];
}
