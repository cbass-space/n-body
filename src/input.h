#pragma once

#include "lib/stb_ds.h"
#include "application.h"

void target_set(Application *application, isize index);
void target_click(Application *application);
void planet_create(Application *application);
void planet_update_gui(Application *application);

#ifdef INPUT_IMPLEMENTATION

void target_click(Application *application) {
    if (application->gui.create) return;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    for (usize i = 0; i < arrlen(application->simulation.planets); i++) {
        Planet planet = application->simulation.planets[i];
        if (CheckCollisionPointCircle(
            GetScreenToWorld2D(GetMousePosition(), application->camera),
            planet.position,
            planet_radius(&application->simulation, planet.mass))
        ) {
            target_set(application, i);
        }
    }
}

void target_set(Application *application, isize index) {
    Planet planet = application->simulation.planets[index];
    application->planet_target = index;
    application->gui.mass = planet.mass;
    application->gui.movable = planet.movable;
    application->gui.color = planet.color;
}

void planet_create(Application *application) {
    if (!application->gui.create) return;
    if (CheckCollisionPointRec(GetMousePosition(), application->gui.layout[0])) return;
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), application->camera);

    Planet *creation = &application->simulation.creation;
    if (GetMouseWheelMove() != 0) {
        f32 scale = 0.2 * GetMouseWheelMove();
        application->gui.mass *= expf(scale);
    }

    Vector2 target_position = (application->planet_target != -1)
        ? application->simulation.planets[application->planet_target].position
        : Vector2Zero();
    Vector2 target_velocity = (application->planet_target != -1)
        ? application->simulation.planets[application->planet_target].velocity
        : Vector2Zero();

    creation->mass = application->gui.mass;
    creation->movable = application->gui.movable;
    creation->color = application->gui.color;

    static Vector2 relative_position = (Vector2) { 0.0, 0.0 };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) relative_position = Vector2Subtract(mouse, target_position);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 dragged_velocity = Vector2Subtract(creation->position, mouse);
        creation->velocity = Vector2Add(dragged_velocity, target_velocity);
        creation->position = Vector2Add(relative_position, target_position);
    } else if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        creation->position = mouse;
        creation->velocity = target_velocity;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        arrpush(application->simulation.planets, *creation);
        creation->velocity = Vector2Zero();
        creation->position = mouse;

        static usize color_index = 0;
        static const Color colors[] = {
            WHITE,
            (Color) { 242, 96, 151, 255 },
            (Color) { 231, 120, 59, 255 },
            (Color) { 182, 153, 39, 255 },
            (Color) { 94, 179, 81, 255 },
            (Color) { 46, 177, 168, 255 },
            (Color) { 55, 167, 222, 255 },
            (Color) { 142, 141, 246, 255 },
        };

        if (application->planet_target == -1) {
            color_index = (color_index + 1) % 8;
            application->gui.color = colors[color_index];
        }
    }
}

// TODO: come up with a better way of doing this
void planet_update_gui(Application *application) {
    if (application->planet_target == -1) return;
    if (application->gui.create) return;

    Planet *planet = &application->simulation.planets[application->planet_target];
    if (application->gui.previous_create) {
        application->gui.mass = planet->mass;
        application->gui.movable = planet->movable;
        application->gui.color = planet->color;
        return;
    }

    planet->mass = application->gui.mass;
    planet->movable = application->gui.movable;
    planet->color = application->gui.color;
}

#endif

