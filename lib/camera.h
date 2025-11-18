#pragma once

#include "types.h"
#include "simulation.h"

void camera_update(SimulationState *simulation, f32 dt);
void target_set(SimulationState *simulation);

#ifdef CAMERA_IMPLEMENTATION

// https://www.youtube.com/watch?v=LSNQuFEDOyQ
Vector2 Vector2ExpDecay(Vector2 a, Vector2 b, f32 decay, f32 dt) {
    return Vector2Add(b, Vector2Scale(
        Vector2Subtract(a, b),
        exp(-decay * dt)
    ));
}

void target_set(SimulationState *simulation) {
    if (simulation->gui.create) return;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    for (usize i = 0; i < simulation->planets.length; i++) {
        Planet planet = simulation->planets.data[i];
        f32 size = simulation->gui.size * pow(planet.mass, 1.0/3.0);
        if (CheckCollisionPointCircle(GetScreenToWorld2D(GetMousePosition(), simulation->camera), planet.position, size)) {
            simulation->target = i;
            simulation->gui.mass = planet.mass;
            simulation->gui.movable = planet.movable;
            simulation->gui.color = planet.color;
        }
    }
}

void camera_update(SimulationState *simulation, f32 dt) {
    #define CAMERA_SMOOTHING 12.0

    Camera2D *camera = &simulation->camera;
    camera->zoom = simulation->camera_zoom_target
        + (camera->zoom - simulation->camera_zoom_target)
        * exp(-CAMERA_SMOOTHING * dt);

    if (simulation->target != -1) {
        camera->target = Vector2ExpDecay(
            camera->target,
            simulation->planets.data[simulation->target].position,
            CAMERA_SMOOTHING,
            dt
        );

        camera->offset = Vector2ExpDecay(
            camera->offset,
            (Vector2) { GetScreenWidth() / 2.0, GetScreenHeight() / 2.0 },
            CAMERA_SMOOTHING,
            dt
        );
    }

    if (CheckCollisionPointRec(GetMousePosition(), simulation->gui.layout[0])) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0 / camera->zoom);
        camera->target = Vector2Add(camera->target, delta);
        simulation->target = -1;
    }

    if (GetMouseWheelMove() != 0.0) {
        if (simulation->gui.create) return;
        f32 scale = 0.2 * GetMouseWheelMove();
        simulation->camera_zoom_target = Clamp(camera->zoom * expf(scale), 0.125, 64.0);

        if (simulation->target == -1) {
            Vector2 mouse_world_position = GetScreenToWorld2D(GetMousePosition(), *camera);
            camera->offset = GetMousePosition();
            camera->target = mouse_world_position;
        }
    }
}

#endif

