#pragma once

#include "simulation.h"
#include "predictor.h"
#include "draw.h"
#include "gui.h"
#include "camera.h"

typedef struct {
    Planet new_planet;
    Visual new_visual;
} InputState;

typedef struct {
    InputState input;
    GUIState gui;
    CameraState camera;
    SimulationState simulation;
    DrawState drawer;
    PredictorState predictor;
} Application;

void application_init(Application *application);
void application_input(Application *application);
void application_update(Application *application, f32 delta_time);
void application_draw(Application *application);
void application_free(Application *application);

#ifdef APP_IMPLEMENTATION

void application_init(Application *application) {
    simulation_init(&application->simulation);
    predictor_init(&application->predictor);
    draw_init(&application->drawer);
    camera_init(&application->camera);
    gui_init(&application->gui);
}

void input_click(Application *application);
void input_keyboard(Application *application);
void input_create(Application *application);
void input_camera(Application *application);
void application_input(Application *application) {
    input_click(application);
    input_keyboard(application);
    input_create(application);
    input_camera(application);
}

void application_update(Application *application, f32 delta_time) {
    if (!application->gui.paused) simulation_update(&application->simulation, delta_time);
    camera_update(&application->camera, &application->simulation, delta_time);
    predictor_update(&application->predictor, &application->simulation, delta_time);
}

void draw_input(Application *application);
void application_draw(Application *application) {
    // draw
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(application->camera.rl_camera);
    draw(&application->drawer, &application->simulation, &application->predictor, &application->camera);
    draw_input(application);
    EndMode2D();

    gui_draw(&application->gui, &application->simulation.parameters, &application->drawer.parameters);
    EndDrawing();
}

void application_free(Application *application) {
    arrfree(application->simulation.planets);
    arrfree(application->drawer.visuals);
    arrfree(application->predictor.predictions);
}

// handle inputs
void target_set(Application *application, isize index);
void input_keyboard(Application *application) {
    if (IsKeyPressed(KEY_R) || application->gui.reset) application_init(application);
    if (IsKeyPressed(KEY_SPACE)) application->gui.paused = !application->gui.paused;
    if (IsKeyPressed(KEY_ESCAPE) && application->gui.create) application->gui.create = false;
    if (IsKeyPressed(KEY_C)) application->gui.create = !application->gui.create;
    if (IsKeyPressed(KEY_M)) application->gui.movable = !application->gui.movable;
    if (IsKeyPressed(KEY_LEFT_BRACKET)) target_set(application, (application->camera.target + arrlen(application->simulation.planets) - 1) % arrlen(application->simulation.planets));
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) target_set(application, (application->camera.target + 1) % arrlen(application->simulation.planets));
    // if (IsKeyPressed(KEY_Z)) application.last_planet = arrpop(application.simulation.planets);
    // if (IsKeyPressed(KEY_Y)) arrpush(application.simulation.planets, application.last_planet);
}

void input_click(Application *application) {
    if (application->gui.create) return;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    for (usize i = 0; i < arrlenu(application->simulation.planets); i++) {
        Planet planet = application->simulation.planets[i];
        if (CheckCollisionPointCircle(
            GetScreenToWorld2D(GetMousePosition(), application->camera.rl_camera),
            planet.position,
            planet_radius(&application->simulation, planet.mass))
        ) {
            target_set(application, i);
        }
    }
}

void target_set(Application *application, isize index) {
    Planet planet = application->simulation.planets[index];
    Visual visuals = application->drawer.visuals[index];

    application->camera.target = index;
    application->gui.mass = planet.mass;
    application->gui.movable = planet.movable;
    application->gui.color = visuals.color;
}

void input_create(Application *application) {
    if (!application->gui.create) return;
    if (CheckCollisionPointRec(GetMousePosition(), application->gui.layout[0])) return;
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), application->camera.rl_camera);

    Planet *creation = &application->input.new_planet;
    Visual *visual = &application->input.new_visual;

    if (GetMouseWheelMove() != 0) {
        f32 scale = 0.2 * GetMouseWheelMove();
        application->gui.mass *= expf(scale);
    }

    Vector2 target_position = (application->camera.target != (usize) -1)
        ? application->simulation.planets[application->camera.target].position
        : Vector2Zero();
    Vector2 target_velocity = (application->camera.target != (usize) -1)
        ? application->simulation.planets[application->camera.target].velocity
        : Vector2Zero();

    creation->mass = application->gui.mass;
    creation->movable = application->gui.movable;
    visual->color = application->gui.color;

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
        arrpush(application->drawer.visuals, *visual);
        arrpush(application->predictor.predictions, (Prediction) { 0 });

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

        if (application->camera.target == (usize) -1) {
            color_index = (color_index + 1) % 8;
            application->gui.color = colors[color_index];
        }
    }
}

void input_camera(Application *application) {
    CameraState *camera = &application->camera;

    if (CheckCollisionPointRec(GetMousePosition(), application->gui.layout[0])) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0 / camera->rl_camera.zoom);
        camera->rl_camera.target = Vector2Add(camera->rl_camera.target, delta);
        camera->target = -1;
    }

    if (GetMouseWheelMove() != 0.0) {
        if (application->gui.create) return;
        f32 scale = 0.2 * GetMouseWheelMove();
        camera->zoom_target = Clamp(camera->zoom_target * expf(scale), 0.125, 64.0);

        if (camera->target == (usize) -1) {
            Vector2 mouse_world_position = GetScreenToWorld2D(GetMousePosition(), camera->rl_camera);
            camera->rl_camera.offset = GetMousePosition();
            camera->rl_camera.target = mouse_world_position;
        }
    }
}

void draw_input(Application *application) {
    if (!application->gui.create) return;
    Planet *planet = &application->input.new_planet;
    Visual *visual = &application->input.new_visual;

    DrawCircleLinesV(planet->position, planet_radius(&application->simulation, planet->mass), visual->color);

    Vector2 target_velocity = (application->camera.target != (usize) -1 && application->drawer.parameters.draw_relative)
        ? application->simulation.planets[application->camera.target].velocity
        : Vector2Zero();
    DrawVector(planet->position, Vector2Subtract(planet->velocity, target_velocity), visual->color);
}

#endif

