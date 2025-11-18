#pragma once

#include "types.h"
#include "arena.h"
#include "raylib.h"
#include "gui.h"

typedef struct {
    Vector2 positions[TRAIL_MAX];
    usize count;
    usize oldest;
} Trail;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    f32 mass;
    bool movable;
    Color color;
    Trail trail;
} Planet;

typedef struct {
    Planet *data;
    usize length;
    usize capacity;
} Planets;

typedef struct {
    GUIState gui;
    Camera2D camera;
    f32 camera_zoom_target;

    Planets planets;
    Planets previous;
    isize target; // index into planets for camera target, -1 if no target

    Vector2 create_position;
} SimulationState;

void simulation_init(SimulationState *simulation, Arena *arena);
void planet_update(SimulationState *simulation, usize index, f32 dt);

typedef struct {
    Vector2 position;
    Vector2 velocity;
} RK4State;

static inline RK4State rk4_add(RK4State a, RK4State b);
static inline RK4State rk4_scale(RK4State state, f32 scale);
static inline RK4State rk4_delta(RK4State state, const SimulationState *simulation, usize index);

#define SIM_IMPLEMENTATION
#ifdef SIM_IMPLEMENTATION

void simulation_init(SimulationState *simulation, Arena *arena) {
    arena_reset(arena);

    simulation->gui = gui_init();
    simulation->camera_zoom_target = 1.0;
    simulation->camera = (Camera2D) {
        .zoom = 1.0,
        .offset = (Vector2) { (f32)GetScreenWidth() / 2.0f, (f32)GetScreenHeight() / 2.0f },
    };

    Planets *planets = &simulation->planets;
    planets->length = 0;
    planets->capacity = 0;
    simulation->target = -1;
}

void planet_update(SimulationState *simulation, usize index, f32 dt) {
    Planet *self = &simulation->planets.data[index];
    if (!self->movable) return;

    RK4State state = {
        .position = self->position,
        .velocity = self->velocity
    };

    RK4State k_1 = rk4_delta(state, simulation, index);
    RK4State k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, dt/2.0)), simulation, index);
    RK4State k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, dt/2.0)), simulation, index);
    RK4State k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, dt)), simulation, index);

    RK4State k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    RK4State new_state = rk4_add(state, rk4_scale(k_sum, dt/6.0));
    self->position = new_state.position;
    self->velocity = new_state.velocity;

    if (!self->movable) return;
    self->trail.positions[(self->trail.oldest + self->trail.count) % TRAIL_MAX] = new_state.position;
    if (self->trail.count < TRAIL_MAX) {
        self->trail.count++;
    } else {
        self->trail.oldest = (self->trail.oldest + 1) % TRAIL_MAX;
    }
}

Vector2 compute_acceleration(Vector2 position, const SimulationState *simulation, usize index) {
    Planet *planet = &simulation->previous.data[index];
    Vector2 displacement = Vector2Subtract(planet->position, position);
    Vector2 direction = Vector2Normalize(displacement);
    f32 length_squared = Vector2LengthSqr(displacement) + EPSILON;
    if (length_squared < EPSILON) { return Vector2Zero(); }
    return Vector2Scale(direction, (simulation->gui.gravity * planet->mass) / length_squared);
}

Vector2 compute_field(Vector2 position, const SimulationState *simulation, usize index) {
    Vector2 net_acceleration = Vector2Zero();
    for (usize i = 0; i < simulation->previous.length; i++) {
        if (i == index) { continue; }
        Vector2 acceleration = compute_acceleration(position, simulation, i);
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
        .velocity = compute_field(state.position, simulation, index),
    };
}

#endif
