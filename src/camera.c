#include "camera.h"
#include "simulation.h"

#include "sdl_utils.h"

void camera_init(Camera *cam) {
    cam->zoom = 1.0f;
    cam->target = (u32) -1;
}

void camera_update(Camera *cam, SDL_Window *window, SDL_GPUDevice *gpu, const Simulation *sim) {
    if (cam->target != (u32) -1) ReadFromGPUBufferNow(gpu, &(ReadGPUBufferBinding) {
        .buffer = sim->positions.buffer,
        .buffer_offset = cam->target * sizeof(HMM_Vec2),
        .destination = (u8*) &cam->position,
        .size = sizeof(HMM_Vec2)
    }, 1);

    i32 width, height;
    SDL_GetWindowSize(window, &width, &height);
    cam->window_size = HMM_V2((f32) width, (f32) height);

    cam->orthographic = HMM_Orthographic_LH_ZO(
        cam->zoom * (-cam->window_size.Width / 2.0f),
        cam->zoom * ( cam->window_size.Width / 2.0f),
        cam->zoom * (-cam->window_size.Height / 2.0f),
        cam->zoom * ( cam->window_size.Height / 2.0f),
        0.0f, 1.0f
    );

    cam->view = HMM_LookAt_LH(
        HMM_V3(cam->position.X, cam->position.Y, 0.0f),
        HMM_V3(cam->position.X, cam->position.Y, 1.0f),
        HMM_V3(0.0f, 1.0f, 0.0f)
    );
}

HMM_Vec2 mouse_world_position(const Camera *cam);
void camera_mouse(Camera *cam, const SDL_Event *event) {
    HMM_Vec2 mouse_delta = { 0 };
    if (SDL_GetRelativeMouseState(&mouse_delta.X, &mouse_delta.Y) & SDL_BUTTON_RMASK) {
        cam->target = (u32) -1;
        mouse_delta.Y *= -1.0f; // screen to world coordinate system
        mouse_delta = HMM_MulV2F(mouse_delta, -cam->zoom);
        cam->position = HMM_AddV2(cam->position, mouse_delta);
    }

    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        const HMM_Vec2 mouse_old = mouse_world_position(cam);
        cam->zoom *= SDL_expf(event->wheel.y * -0.1f);

        if (cam->target == (u32) -1) {
            const HMM_Vec2 mouse_new = mouse_world_position(cam);
            const HMM_Vec2 delta = HMM_SubV2(mouse_new, mouse_old);
            cam->position = HMM_SubV2(cam->position, delta);
        }
    }
}

void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim) {
    if (event->type != SDL_EVENT_KEY_DOWN) return;
    if (event->key.scancode == SDL_SCANCODE_RIGHTBRACKET) cam->target = (cam->target + 1) % sim->body_count;
    if (event->key.scancode == SDL_SCANCODE_LEFTBRACKET) cam->target = (cam->target - 1 + sim->body_count) % sim->body_count;
}

HMM_Vec2 screen_to_world(const Camera *cam, HMM_Vec2 position) {
    const HMM_Vec2 center = HMM_V2(cam->window_size.Width / 2.0f, cam->window_size.Height / 2.0f);
    position.Y = cam->window_size.Y - position.Y;

    HMM_Vec2 camera_to_point = HMM_SubV2(position, center);
    camera_to_point = HMM_MulV2F(camera_to_point, cam->zoom);
    return HMM_AddV2(cam->position, camera_to_point);
}

HMM_Vec2 world_to_screen(const Camera *cam, const HMM_Vec2 position) {
    const HMM_Vec2 center = HMM_V2(cam->window_size.Width / 2.0f, cam->window_size.Height / 2.0f);

    HMM_Vec2 camera_to_point = HMM_SubV2(position, cam->position);
    camera_to_point = HMM_DivV2F(camera_to_point, cam->zoom);
    HMM_Vec2 screen = HMM_AddV2(center, camera_to_point);
    screen.Y = cam->window_size.Height - screen.Y;
    return screen;
}

HMM_Vec2 mouse_world_position(const Camera *cam) {
    HMM_Vec2 mouse = { 0 };
    SDL_GetMouseState(&mouse.X, &mouse.Y);
    return screen_to_world(cam, mouse);
}

