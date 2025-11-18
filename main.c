#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#define ARENA_IMPLEMENTATION
#define GUI_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#define DRAW_IMPLEMENTATION
#define CAMERA_IMPLEMENTATION

#include "lib/types.h"
#include "lib/arena.h"
#include "lib/gui.h"
#include "lib/simulation.h"
#include "lib/draw.h"
#include "lib/camera.h"

#define MAX_LENGTH 2048
#define DEFAULT_WIDTH 1200
#define DEFAULT_HEIGHT 900

void planet_create(SimulationState *simulation, Arena *arena);
void planet_update_gui(SimulationState *simulation);

i32 main() {
    Arena arena = arena_new(1024 * 1024);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "esby is confused");
    SetTargetFPS(60);

    SimulationState simulation;
    simulation_init(&simulation, &arena);

    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        f32 dt = GetFrameTime();

        if (IsKeyPressed(KEY_R) || simulation.gui.reset) simulation_init(&simulation, &arena);
        if (IsKeyPressed(KEY_SPACE)) simulation.gui.paused = !simulation.gui.paused;
        if (IsKeyPressed(KEY_ESCAPE) && simulation.gui.create) simulation.gui.create = false;
        if (IsKeyPressed(KEY_LEFT_BRACKET)) simulation.target = (simulation.target - 1) % simulation.planets.length;
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) simulation.target = (simulation.target + 1) % simulation.planets.length;
        if (IsKeyPressed(KEY_C)) simulation.gui.create = !simulation.gui.create;
        if (IsKeyPressed(KEY_M)) simulation.gui.movable = !simulation.gui.movable;

        target_set(&simulation);
        camera_update(&simulation, dt);
        planet_update_gui(&simulation);

        planet_create(&simulation, &arena);

        simulation.previous.length = simulation.planets.length;
        memcpy(simulation.previous.data, simulation.planets.data, simulation.planets.length * sizeof(Planet));

        for (usize i = 0; i < simulation.planets.length; i++) {
            if (!simulation.gui.paused) planet_update(&simulation, i, dt);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(simulation.camera);

        for (usize i = 0; i < simulation.planets.length; i++) {
            planet_draw(&simulation, i);
        }

        planet_new_draw(&simulation);
        field_grid_draw(&simulation);

        EndMode2D();
        gui_draw(&simulation.gui);
        EndDrawing();
    }

    CloseWindow();
    arena_free(&arena);
    return 0;
}

void planet_create(SimulationState *simulation, Arena *arena) {
    if (!simulation->gui.create) return;
    if (CheckCollisionPointRec(GetMousePosition(), simulation->gui.layout[0])) return;
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), simulation->camera);

    if (GetMouseWheelMove() != 0) {
        f32 scale = 0.2 * GetMouseWheelMove();
        simulation->gui.mass = expf(logf(simulation->gui.mass) + scale);
    }

    Vector2 target_position = (simulation->target != -1)
        ? simulation->planets.data[simulation->target].position
        : Vector2Zero();
    Vector2 target_velocity = (simulation->target != -1)
        ? simulation->planets.data[simulation->target].velocity
        : Vector2Zero();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) simulation->create_position = Vector2Subtract(mouse, target_position);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        Vector2 absolute_position = Vector2Add(target_position, simulation->create_position);
        Vector2 dragged_velocity = Vector2Subtract(absolute_position, mouse); // like angry birds

        *list_push(&simulation->previous, arena) = (Planet) { 0 };
        *list_push(&simulation->planets, arena) = (Planet) {
            .position = absolute_position,
            .velocity = Vector2Add(target_velocity, dragged_velocity),
            .mass = simulation->gui.mass,
            .movable = simulation->gui.movable,
            .color = simulation->gui.color,
        };
    }
}

void planet_update_gui(SimulationState *simulation) {
    if (simulation->target == -1) return;
    if (simulation->gui.create) return;

    Planet *planet = &simulation->planets.data[simulation->target];
    if (simulation->gui.previous_create) {
        simulation->gui.mass = planet->mass;
        simulation->gui.movable = planet->movable;
        simulation->gui.color = planet->color;
        return;
    }

    planet->mass = simulation->gui.mass;
    planet->movable = simulation->gui.movable;
    planet->color = simulation->gui.color;
}

