#pragma once

#include "lib/types.h"
#include "application.h"
#include "src/simulation.h"

void planet_draw(const Application *application, usize index);
void planet_creation_draw(const Application *application);
void field_grid_draw(const Application *application);

#ifdef DRAW_IMPLEMENTATION

void DrawVector(Vector2 start, Vector2 vector, Color color) {
    #define ARROW_HEAD_LENGTH 10
    #define ARROW_ANGLE 3*PI/4

    if (Vector2LengthSqr(vector) < EPSILON) return;
    
    Vector2 end = Vector2Add(start, vector);
    DrawLineV(start, end, color);
    
    Vector2 head_1 = Vector2Normalize(Vector2Rotate(vector, ARROW_ANGLE));
    Vector2 head_2 = Vector2Normalize(Vector2Rotate(vector, -ARROW_ANGLE));
    DrawLineV(end, Vector2Add(end, Vector2Scale(head_1, ARROW_HEAD_LENGTH)), color);
    DrawLineV(end, Vector2Add(end, Vector2Scale(head_2, ARROW_HEAD_LENGTH)), color);
}

void planet_draw(const Application *application, usize index) {
    Planet *planet = &application->simulation.planets[index];

    void (*draw)(Vector2, f32, Color) = planet->movable ? DrawCircleLinesV : DrawCircleV;
    draw(
        planet->position,
        planet_radius(&application->simulation, planet->mass),
        planet->color
    );

    // TODO: slider to control force arrow size?
    if (application->gui.draw_forces) {
        for (usize i = 0; i < arrlen(application->simulation.planets); i++) {
            if (i == index) continue;
            Vector2 accleration = compute_acceleration(&application->simulation, planet->position, i, -1);
            Vector2 force = Vector2Scale(accleration, planet->mass);
            DrawVector(planet->position, force, planet->color);
        }
    }

    // TODO: reduce computation by caching in update loop?
    if (application->gui.draw_net_force) {
        Vector2 net_acceleration = compute_net_acceleration(&application->simulation, planet->position, index, -1);
        Vector2 net_force = Vector2Scale(net_acceleration, planet->mass);
        DrawVector(planet->position, net_force, planet->color);
    }

    if (!planet->movable && application->planet_target == -1) return;
    Vector2 target_velocity = (application->gui.draw_relative && application->planet_target != -1)
        ? application->simulation.planets[application->planet_target].velocity
        : Vector2Zero();
    if (application->gui.draw_velocity) DrawVector(planet->position, Vector2Subtract(planet->velocity, target_velocity), planet->color);

    // TODO: find a way to use DrawLineStrip with variable color?
    usize trail_count = planet->trail.count < application->gui.trail ? planet->trail.count : application->gui.trail;
    for (usize i = 1; i < trail_count; i++) {
        Vector2 a = planet->trail.positions[trail_nth_latest(&planet->trail, i)];
        Vector2 b = planet->trail.positions[trail_nth_latest(&planet->trail, i + 1)];

        if (application->gui.draw_relative && application->planet_target != -1) {
            Planet *target = &application->simulation.planets[application->planet_target];
            a = Vector2Add(target->position, Vector2Subtract(a, target->trail.positions[trail_nth_latest(&target->trail, i)]));
            b = Vector2Add(target->position, Vector2Subtract(b, target->trail.positions[trail_nth_latest(&target->trail, i + 1)]));
        }

        Color color = planet->color;
        color.a = 256 * (trail_count - i) / trail_count;
        DrawLineV(a, b, color);
    }

    usize predict_count = PREDICT_LENGTH;
    for (usize i = 1; i < predict_count; i++) {
        Vector2 a = planet->prediction.positions[i];
        Vector2 b = planet->prediction.positions[i - 1];

        if (application->gui.draw_relative && application->planet_target != -1) {
            Planet *target = &application->simulation.planets[application->planet_target];
            a = Vector2Add(target->position, Vector2Subtract(a, target->prediction.positions[i]));
            b = Vector2Add(target->position, Vector2Subtract(b, target->prediction.positions[i - 1]));
        }

        Color color = planet->color;
        color.a = 256 * (predict_count - i) / (3.0 * predict_count);
        DrawLineV(a, b, color);
    }
}

void planet_creation_draw(const Application *application) {
    if (!application->gui.create) return;
    if (CheckCollisionPointRec(GetMousePosition(), application->gui.layout[0])) return;
    const Planet *creation = &application->simulation.creation;

    Color color = creation->color;
    color.a = 128;

    void (*draw)(Vector2, f32, Color) = creation->movable ? DrawCircleLinesV : DrawCircleV;
    draw(creation->position, planet_radius(&application->simulation, creation->mass), color);

    if (!creation->movable && application->planet_target == -1) return;
    Vector2 target_velocity = (application->planet_target != -1 && application->gui.draw_relative)
        ? application->simulation.planets[application->planet_target].velocity
        : Vector2Zero();
    DrawVector(
        creation->position,
        Vector2Subtract(creation->velocity, target_velocity),
        application->gui.color
    );
};

void field_grid_draw(const Application *application) {
    if (!application->gui.draw_field_grid) return;

    Vector2 start = GetScreenToWorld2D(Vector2Zero(), application->camera);
    Vector2 end = GetScreenToWorld2D((Vector2) { (f32)GetScreenWidth(), (f32)GetScreenHeight() }, application->camera);
    
    for (f32 x = start.x - fmodf(start.x, GRID_DEFAULT); x < end.x + GRID_DEFAULT; x += GRID_DEFAULT) {
        for (f32 y = start.y - fmodf(start.y, GRID_DEFAULT); y < end.y + GRID_DEFAULT; y += GRID_DEFAULT) {
            Vector2 position = (Vector2) { x, y };
            Vector2 acceleration = compute_net_acceleration(&application->simulation, position, -1, -1);
            DrawVector(position, acceleration, DARKGRAY);
        }
    }
}

#endif
