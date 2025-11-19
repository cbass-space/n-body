#pragma once

#include "lib/types.h"
#include "simulation.h"

void planet_draw(SimulationState *simulation, usize index);
void planet_new_draw(SimulationState *simulation);
void field_grid_draw(SimulationState *simulation);

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

void planet_draw(SimulationState *simulation, usize index) {
    Planet *planet = &simulation->planets.data[index];

    void (*draw)(Vector2, f32, Color) = planet->movable ? DrawCircleLinesV : DrawCircleV;
    draw(
        planet->position,
        planet_radius(simulation, planet->mass),
        planet->color
    );

    // slider to control force arrow size?
    if (simulation->gui.draw_forces) {
        for (usize i = 0; i < simulation->planets.length; i++) {
            if (i == index) continue;
            Vector2 accleration = compute_acceleration(planet->position, simulation, i);
            Vector2 force = Vector2Scale(accleration, planet->mass);
            DrawVector(planet->position, force, simulation->planets.data[i].color);
        }

    }

    // way to reduce computation?
    if (simulation->gui.draw_net_force) {
        Vector2 net_acceleration = compute_net_acceleration(planet->position, simulation, index);
        Vector2 net_force = Vector2Scale(net_acceleration, planet->mass);
        DrawVector(planet->position, net_force, planet->color);
    }

    if (!planet->movable && simulation->target == -1) return;
    Vector2 target_velocity = (simulation->gui.draw_relative && simulation->target != -1)
        ? simulation->planets.data[simulation->target].velocity
        : Vector2Zero();
    if (simulation->gui.draw_velocity) DrawVector(planet->position, Vector2Subtract(planet->velocity, target_velocity), planet->color);

    usize count = planet->trail.count < simulation->gui.trail ? planet->trail.count : simulation->gui.trail;
    for (usize i = 1; i < count; i++) {
        Vector2 a = planet->trail.positions[(planet->trail.oldest + planet->trail.count - i) % TRAIL_MAX];
        Vector2 b = planet->trail.positions[(planet->trail.oldest + planet->trail.count - i - 1) % TRAIL_MAX];

        if (simulation->gui.draw_relative && simulation->target != -1) {
            Planet *target = &simulation->planets.data[simulation->target];
            a = Vector2Add(target->position, Vector2Subtract(a, target->trail.positions[(target->trail.oldest + target->trail.count - i) % TRAIL_MAX]));
            b = Vector2Add(target->position, Vector2Subtract(b, target->trail.positions[(target->trail.oldest + target->trail.count - i - 1) % TRAIL_MAX]));
        }

        Color color = planet->color;
        color.a = 256 * (count - i) / count;
        DrawLineV(a, b, color);
    }
}

void planet_new_draw(SimulationState *simulation) {
    if (!simulation->gui.create) return;
    if (CheckCollisionPointRec(GetMousePosition(), simulation->gui.layout[0])) return;
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), simulation->camera);

    void (*draw)(Vector2, f32, Color) = simulation->gui.movable ? DrawCircleLinesV : DrawCircleV;
    Color color = simulation->gui.color;
    color.a = 128;

    Vector2 target_position = (simulation->target != -1)
        ? simulation->planets.data[simulation->target].position
        : Vector2Zero();
    Vector2 target_velocity = (simulation->target != -1 && !simulation->gui.draw_relative)
        ? simulation->planets.data[simulation->target].velocity
        : Vector2Zero();
    Vector2 position = IsMouseButtonDown(MOUSE_BUTTON_LEFT)
        ? Vector2Add(simulation->create_position, target_position)
        : mouse;
    draw(position, planet_radius(simulation, simulation->gui.mass), color);

    if (!simulation->gui.movable) return;
    Vector2 dragged_velocity = IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? Vector2Subtract(position, mouse) : Vector2Zero();
    DrawVector(
        position,
        Vector2Add(target_velocity, dragged_velocity),
        simulation->gui.color
    );
};

void field_grid_draw(SimulationState *simulation) {
    if (!simulation->gui.draw_field_grid) return;

    Vector2 start = GetScreenToWorld2D(Vector2Zero(), simulation->camera);
    Vector2 end = GetScreenToWorld2D((Vector2) { (f32)GetScreenWidth(), (f32)GetScreenHeight() }, simulation->camera);
    
    for (f32 x = start.x - fmodf(start.x, GRID_DEFAULT); x < end.x + GRID_DEFAULT; x += GRID_DEFAULT) {
        for (f32 y = start.y - fmodf(start.y, GRID_DEFAULT); y < end.y + GRID_DEFAULT; y += GRID_DEFAULT) {
            Vector2 position = (Vector2) { x, y };
            Vector2 acceleration = compute_net_acceleration(position, simulation, -1);
            DrawVector(position, acceleration, DARKGRAY);
        }
    }
}

#endif
