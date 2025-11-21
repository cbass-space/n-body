#pragma once

#include "simulation.h"
#include "gui.h"

typedef struct {
    Simulation simulation;
    isize planet_target;
    GUI gui;
    Camera2D camera;
    f32 camera_zoom_target;
} Application;

void application_init(Application *application);

#ifdef APP_IMPLEMENTATION

void application_init(Application *application) {
    application->planet_target = -1;
    simulation_init(&application->simulation);

    application->gui = gui_init(&application->simulation.parameters);
    application->camera_zoom_target = 1.0;
    application->camera = (Camera2D) {
        .offset = (Vector2) { (f32)GetScreenWidth() / 2.0f, (f32)GetScreenHeight() / 2.0f },
        .zoom = 1.0,
    };
}

#endif

