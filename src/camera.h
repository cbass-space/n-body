#pragma once

#include "lib/types.h"
#include "application.h"

void camera_update(Application *application, f32 dt);

#ifdef CAMERA_IMPLEMENTATION

// https://www.youtube.com/watch?v=LSNQuFEDOyQ
Vector2 Vector2ExpDecay(Vector2 a, Vector2 b, f32 decay, f32 dt) {
    return Vector2Add(b, Vector2Scale(
        Vector2Subtract(a, b),
        exp(-decay * dt)
    ));
}

void camera_update(Application *application, f32 dt) {
    #define CAMERA_SMOOTHING 12.0

    Camera2D *camera = &application->camera;
    camera->zoom = application->camera_zoom_target
        + (camera->zoom - application->camera_zoom_target)
        * exp(-CAMERA_SMOOTHING * dt);

    if (application->planet_target != -1) {
        camera->target = Vector2ExpDecay(
            camera->target,
            application->simulation.planets[application->planet_target].position,
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

    if (CheckCollisionPointRec(GetMousePosition(), application->gui.layout[0])) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0 / camera->zoom);
        camera->target = Vector2Add(camera->target, delta);
        application->planet_target = -1;
    }

    if (GetMouseWheelMove() != 0.0) {
        if (application->gui.create) return;
        f32 scale = 0.2 * GetMouseWheelMove();
        application->camera_zoom_target = Clamp(camera->zoom * expf(scale), 0.125, 64.0);

        if (application->planet_target == -1) {
            Vector2 mouse_world_position = GetScreenToWorld2D(GetMousePosition(), *camera);
            camera->offset = GetMousePosition();
            camera->target = mouse_world_position;
        }
    }
}

#endif

