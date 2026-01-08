#include "main.h"

void ghost_init(Ghost *ghost) {
    *ghost = (Ghost) {
        .mode = false,
        .mass = MASS_DEFAULT,
        .movable = true,
        .color = COLOR_DEFAULT
    };
}

void ghost_update(Ghost *ghost, SDL_Window *window, const Simulation *sim, const Camera *cam) {
    if (!ghost->mode) return;

    const HMM_Vec2 mouse = mouse_world_position(cam, window);
    const HMM_Vec2 target_position = (cam->target != (usize) -1)
        ? sim->r[cam->target]
        : (HMM_Vec2) { 0 };
    const HMM_Vec2 target_velocity = (cam->target != (usize) -1)
        ? sim->v[cam->target]
        : (HMM_Vec2) { 0 };

    ghost->position = HMM_AddV2(target_position, ghost->relative_position);
    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK) {
        const HMM_Vec2 dragged_velocity = HMM_SubV2(ghost->position, mouse);
        ghost->velocity = HMM_AddV2(target_velocity, dragged_velocity);
    } else {
        ghost->relative_position = HMM_SubV2(mouse, target_position);
        ghost->velocity = target_velocity;
    };
}

bool ghost_mouse(Ghost *ghost, const SDL_Event *event) {
    if (!ghost->mode) return false;
    if (event->type == SDL_EVENT_MOUSE_WHEEL) ghost->mass *= SDL_expf(event->wheel.y * -0.1f);
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) return true;
    return false;
}

void ghost_keyboard(Ghost *ghost, const SDL_Event *event) {
    if (event->type != SDL_EVENT_KEY_DOWN) return;
    if (event->key.scancode == SDL_SCANCODE_C) ghost->mode = !ghost->mode;
    if (event->key.scancode == SDL_SCANCODE_M) ghost->movable = !ghost->movable;
}
