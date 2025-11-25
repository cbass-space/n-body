#pragma once

#include "lib/types.h"
#include "raylib.h"
#include "raymath.h"

#include "simulation.h"

typedef struct {
    Camera2D rl_camera;
    f32 zoom_target;
    usize target;
} CameraState;

void camera_init(CameraState *camera);
void camera_update(CameraState *camera, SimulationState *simulation, f32 delta_time);

#ifdef CAMERA_IMPLEMENTATION

void camera_init(CameraState *camera) {
    camera->rl_camera = (Camera2D) { .zoom = 1.0 };
    camera->zoom_target = 1.0;
    camera->target = (usize) -1;
}

Vector2 Vector2ExpDecay(Vector2 a, Vector2 b, f32 decay, f32 delta_time);
void camera_update(CameraState *camera, SimulationState *simulation, f32 delta_time) {
    #define CAMERA_SMOOTHING 12.0

    Camera2D *rl_camera = &camera->rl_camera;
    rl_camera->zoom = camera->zoom_target
        + (rl_camera->zoom - camera->zoom_target)
        * exp(-CAMERA_SMOOTHING * delta_time);

    if (camera->target != (usize) -1) {
        rl_camera->target = Vector2ExpDecay(
            rl_camera->target,
            simulation->planets[camera->target].position,
            CAMERA_SMOOTHING,
            delta_time
        );

        rl_camera->offset = Vector2ExpDecay(
            rl_camera->offset,
            (Vector2) { GetScreenWidth() / 2.0, GetScreenHeight() / 2.0 },
            CAMERA_SMOOTHING,
            delta_time
        );
    }
}

// https://www.youtube.com/watch?v=LSNQuFEDOyQ
Vector2 Vector2ExpDecay(Vector2 a, Vector2 b, f32 decay, f32 delta_time) {
    return Vector2Add(b, Vector2Scale(
        Vector2Subtract(a, b),
        exp(-decay * delta_time)
    ));
}

#endif

