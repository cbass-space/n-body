#include "ghost.h"
#include "constants.h"
#include "simulation.h"
#include "camera.h"

#include "sdl_utils.h"

void ghost_init(Ghost *ghost) {
    *ghost = (Ghost) {
        .enabled = false,
        .mass = MASS_DEFAULT,
        .movable = true,
        .color = COLOR_DEFAULT
    };
}

void ghost_update(Ghost *ghost, SDL_GPUDevice *gpu, const Simulation *sim, const Camera *cam) {
    if (!ghost->enabled) return;

    HMM_Vec2 target_position = HMM_V2(0.0f, 0.0f);
    HMM_Vec2 target_velocity = HMM_V2(0.0f, 0.0f);
    if (cam->target != (u32) -1) {
        ReadGPUBufferBinding bindings[] = {
            {
                .buffer = sim->positions.buffer,
                .buffer_offset = cam->target * sizeof(HMM_Vec2),
                .destination = (u8*) &target_position,
                .size = sizeof(HMM_Vec2)
            },
            {
                .buffer = sim->velocities.buffer,
                .buffer_offset = cam->target * sizeof(HMM_Vec2),
                .destination = (u8*) &target_position,
                .size = sizeof(HMM_Vec2)
            },
        };

        ReadFromGPUBufferNow(gpu, bindings, 2);
    }

    const HMM_Vec2 mouse = mouse_world_position(cam);
    ghost->position = HMM_AddV2(target_position, ghost->relative_position);
    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK) {
        const HMM_Vec2 dragged_velocity = HMM_SubV2(ghost->position, mouse);
        ghost->velocity = HMM_AddV2(target_velocity, dragged_velocity);
    } else {
        ghost->relative_position = HMM_SubV2(mouse, target_position);
        ghost->velocity = target_velocity;
    }
}

bool ghost_mouse(Ghost *ghost, const SDL_Event *event) {
    if (!ghost->enabled) return false;
    if (event->type == SDL_EVENT_MOUSE_WHEEL) ghost->mass *= SDL_expf(event->wheel.y * -0.1f);
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) return true;
    return false;
}

void ghost_keyboard(Ghost *ghost, const SDL_Event *event) {
    if (event->type != SDL_EVENT_KEY_DOWN) return;
    if (event->key.scancode == SDL_SCANCODE_C) ghost->enabled = !ghost->enabled;
    if (event->key.scancode == SDL_SCANCODE_M) ghost->movable = !ghost->movable;
}

