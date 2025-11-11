#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"
#include "lib/types.h"
#include "lib/arena.h"
#include "lib/list.h"

#include "gui.h"

#define MAX_LENGTH 2048
#define WIDTH 1200
#define HEIGHT 900

typedef struct {
    f32 mass;
    Vector2 position;
    Vector2 velocity;
    Color color;
    bool movable;
} Planet;

typedef struct {
    Planet *data;
    usize length;
    usize capacity;
} Planets;

typedef struct {
    bool paused;
    f32 gravity;
    Planets *planets;
    Planets *previous;
} SimulationState;

void planets_init(Planets *planets, Arena *arena);
void planet_update(SimulationState *simulation, usize index, f32 dt);
void camera_update(Camera2D *camera, Rectangle gui_rectangle);

i32 main() {
    Arena arena = arena_new(1024 * 1024);

    InitWindow(WIDTH, HEIGHT, "esby is confused");
    SetTargetFPS(60);
    GUIState gui = init_gui();

    Camera2D camera = { .zoom = 1.0 };

    Planets planets = { 0 };
    Planets previous = { 0 };
    planets_init(&planets, &arena);
    SimulationState sim_state = {
        .paused = false,
        .gravity = gui.gravity,
        .planets = &planets,
        .previous = &previous
    };

    while (!WindowShouldClose()) {
        f32 dt = GetFrameTime();

        sim_state.gravity = gui.gravity;
        sim_state.paused = gui.paused;

        Planet previous_data[sim_state.planets->length];
        memcpy(previous_data, sim_state.planets->data, sizeof(previous_data));
        *sim_state.previous = (Planets) { &previous_data, sim_state.planets->length, sim_state.planets->capacity };

        for (usize i = 0; i < sim_state.planets->length; i++) {
            if (!sim_state.paused) planet_update(&sim_state, i, dt);
        }

        if (gui.reset) planets_init(sim_state.planets, &arena);

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        for (usize i = 0; i < sim_state.planets->length; i++) {
            Planet *planet = &sim_state.planets->data[i];
            DrawCircleLinesV(
                planet->position,
                gui.size * pow(planet->mass, 1.0/3.0),
                planet->color
            );
        }

        camera_update(&camera, gui.layout[0]);

        EndMode2D();
        draw_gui(&gui);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

typedef struct {
    Vector2 position;
    Vector2 velocity;
} RK4State;

static inline RK4State rk4_add(RK4State a, RK4State b);
static inline RK4State rk4_scale(RK4State state, f32 scale);
static inline RK4State rk4_delta(RK4State state, const SimulationState *simulation, usize index);

void planet_update(SimulationState *simulation, usize index, f32 dt) {
    Planet *self = &simulation->planets->data[index];
    if (!self->movable) return;
    RK4State state = {
        .position = self->position,
        .velocity = self->velocity
    };

    RK4State k_1 = rk4_delta(state, simulation, index);
    RK4State k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, dt/2.0)), simulation, index);
    RK4State k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, dt/2.0)), simulation, index);
    RK4State k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, dt)), simulation, index);

    // y_(n+1) = y_n + (dt/6)(k_1 + 2k_2 + 2k_3 + k_4)
    RK4State k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    RK4State new_state = rk4_add(state, rk4_scale(k_sum, dt/6.0));
    self->position = new_state.position;
    self->velocity = new_state.velocity;
}

static Vector2 compute_acceleration(Vector2 position, const SimulationState *simulation, usize index) {
    Vector2 net_acceleration = Vector2Zero();
    for (usize i = 0; i < simulation->previous->length; i++) {
        if (i == index) { continue; }
        Planet *planet = &simulation->previous->data[i];

        Vector2 displacement = Vector2Subtract(planet->position, position);
        Vector2 direction = Vector2Normalize(displacement);
        f32 length_squared = Vector2LengthSqr(displacement);
        if (length_squared < EPSILON) { continue; }

        Vector2 acceleration = Vector2Scale(direction, (simulation->gravity * planet->mass) / length_squared);
        net_acceleration = Vector2Add(net_acceleration, acceleration);
    }

    return net_acceleration;
}

static inline RK4State rk4_add(RK4State a, RK4State b) {
    return (RK4State) {
        .position = Vector2Add(a.position, b.position),
        .velocity = Vector2Add(a.velocity, b.velocity),
    };
}

static inline RK4State rk4_scale(RK4State state, f32 scale) {
    return (RK4State) {
        .position = Vector2Scale(state.position, scale),
        .velocity = Vector2Scale(state.velocity, scale),
    };
}

static inline RK4State rk4_delta(RK4State state, const SimulationState *simulation, usize index) {
    return (RK4State) {
        .position = state.velocity,
        .velocity = compute_acceleration(state.position, simulation, index),
    };
}

void planets_init(Planets *planets, Arena *arena) {
    planets->length = 0;
    planets->capacity = 0;

    *list_push(planets, arena) = (Planet) {
        .mass = 100.0,
        .position = (Vector2) { WIDTH / 3.0, HEIGHT / 3.0 },
        .velocity = (Vector2) { 10.0, 60.0 },
        .color = WHITE,
        .movable = true,
    };

    *list_push(planets, arena) = (Planet) {
        .mass = 100.0,
        .position = (Vector2) { 2.0 * WIDTH / 3.0, HEIGHT / 3.0 },
        .velocity = (Vector2) { -80.0, 10.0 },
        .color = WHITE,
        .movable = true,
    };

    *list_push(planets, arena) = (Planet) {
        .mass = 100.0,
        .position = (Vector2) { WIDTH / 2.0, 2.0 * HEIGHT / 3.0 },
        .velocity = (Vector2) { 50.0, -10.0 },
        .color = WHITE,
        .movable = true,
    };
}

void camera_update(Camera2D *camera, Rectangle gui_rectangle) {
    if (CheckCollisionPointRec(GetMousePosition(), gui_rectangle)) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0 / camera->zoom);
        camera->target = Vector2Add(camera->target, delta);
    }

    f32 wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouse_world_position = GetScreenToWorld2D(GetMousePosition(), *camera);
        camera->offset = GetMousePosition();
        camera->target = mouse_world_position;
        f32 scale = 0.2 * wheel;
        camera->zoom = Clamp(expf(logf(camera->zoom) + scale), 0.125, 64.0);
    }
}
