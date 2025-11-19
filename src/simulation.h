#pragma once

#include "raylib.h"
#include "raymath.h"

#include "lib/types.h"
#include "lib/arena.h"

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

f32 planet_radius(SimulationState *simulation, f32 mass);
void simulation_init(SimulationState *simulation, Arena *arena);
void planet_update(SimulationState *simulation, usize index, f32 dt);

#ifdef SIM_IMPLEMENTATION

f32 planet_radius(SimulationState *simulation, f32 mass) {
    return powf(mass / simulation->gui.density, 1.0/3.0);
}

void simulation_init(SimulationState *simulation, Arena *arena) {
    arena_reset(arena);

    simulation->gui = gui_init();
    simulation->camera_zoom_target = 1.0;
    simulation->camera = (Camera2D) {
        .offset = (Vector2) { (f32)GetScreenWidth() / 2.0f, (f32)GetScreenHeight() / 2.0f },
        .zoom = 1.0,
    };

    simulation->previous.length = simulation->planets.length = 0;
    simulation->previous.capacity = simulation->planets.capacity = 0;
    simulation->target = -1;
}

typedef struct {
    Vector2 position;
    Vector2 velocity;
} PlanetState;
PlanetState euler_update(const SimulationState *simulation, usize index, f32 dt);
PlanetState verlet_update(const SimulationState *simulation, usize index, f32 dt);
PlanetState rk4_update(const SimulationState *simulation, usize index, f32 dt);
PlanetState (*integrators[])(const SimulationState*, usize, f32) = { euler_update, verlet_update, rk4_update };

void planet_update(SimulationState *simulation, usize index, f32 dt) {
    if (simulation->gui.paused) return; // TODO: better place to put this?
    Planet *planet = &simulation->planets.data[index];
    if (!planet->movable) return;

    PlanetState new_state = integrators[simulation->gui.integrator](simulation, index, dt);
    planet->position = new_state.position;
    planet->velocity = new_state.velocity;

    planet->trail.positions[(planet->trail.oldest + planet->trail.count) % TRAIL_MAX] = planet->position;
    if (planet->trail.count < TRAIL_MAX) {
        planet->trail.count++;
    } else {
        planet->trail.oldest = (planet->trail.oldest + 1) % TRAIL_MAX;
    }
}

Vector2 compute_acceleration(Vector2 position, const SimulationState *simulation, usize index) {
    Planet *planet = &simulation->previous.data[index];
    Vector2 displacement = Vector2Subtract(planet->position, position);
    Vector2 direction = Vector2Normalize(displacement);
    f32 length_squared = Vector2LengthSqr(displacement) + powf(simulation->gui.softening, 2.0);
    if (length_squared < EPSILON) { return Vector2Zero(); }
    return Vector2Scale(direction, (simulation->gui.gravity * planet->mass) / length_squared);
}

Vector2 compute_net_acceleration(Vector2 position, const SimulationState *simulation, usize index) {
    Vector2 net_acceleration = Vector2Zero();
    for (usize i = 0; i < simulation->previous.length; i++) {
        if (i == index) { continue; }
        Vector2 acceleration = compute_acceleration(position, simulation, i);
        net_acceleration = Vector2Add(net_acceleration, acceleration);
    }

    return net_acceleration;
}

// https://gafferongames.com/post/integration_basics/#semi-implicit-euler
PlanetState euler_update(const SimulationState *simulation, usize index, f32 dt) {
    Planet *planet = &simulation->planets.data[index];
    Vector2 acceleration = compute_net_acceleration(planet->position, simulation, index);
    Vector2 velocity_new = Vector2Add(planet->velocity, Vector2Scale(acceleration, dt));
    Vector2 position_new = Vector2Add(planet->position, Vector2Scale(velocity_new, dt));

    return (PlanetState) {
        .velocity = velocity_new,
        .position = position_new
    };
}

// https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
PlanetState verlet_update(const SimulationState *simulation, usize index, f32 dt) {
    Planet *planet = &simulation->planets.data[index];

    Vector2 acceleration = compute_net_acceleration(planet->position, simulation, index);
    Vector2 position_new = Vector2Add(
        Vector2Add(planet->position, Vector2Scale(planet->velocity, dt)),
        Vector2Scale(acceleration, powf(dt, 2.0) / 2.0)
    );

    Vector2 acceleration_new = compute_net_acceleration(position_new, simulation, index);
    Vector2 velocity_new = Vector2Add(
        planet->velocity,
        Vector2Scale(Vector2Add(acceleration, acceleration_new), dt / 2.0)
    );

    return (PlanetState){
        .position = position_new,
        .velocity = velocity_new
    };
}

static inline PlanetState rk4_add(PlanetState a, PlanetState b);
static inline PlanetState rk4_scale(PlanetState state, f32 scale);
static inline PlanetState rk4_delta(PlanetState state, const SimulationState *simulation, usize index);

// https://en.wikipedia.org/wiki/Runge-Kutta_methods
PlanetState rk4_update(const SimulationState *simulation, usize index, f32 dt) {
    Planet *planet = &simulation->planets.data[index];
    PlanetState state = {
        .position = planet->position,
        .velocity = planet->velocity
    };

    PlanetState k_1 = rk4_delta(state, simulation, index);
    PlanetState k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, dt/2.0)), simulation, index);
    PlanetState k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, dt/2.0)), simulation, index);
    PlanetState k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, dt)), simulation, index);

    PlanetState k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    return rk4_add(state, rk4_scale(k_sum, dt/6.0));
}

static inline PlanetState rk4_add(PlanetState a, PlanetState b) {
    return (PlanetState) {
        .position = Vector2Add(a.position, b.position),
        .velocity = Vector2Add(a.velocity, b.velocity),
    };
}

static inline PlanetState rk4_scale(PlanetState state, f32 scale) {
    return (PlanetState) {
        .position = Vector2Scale(state.position, scale),
        .velocity = Vector2Scale(state.velocity, scale),
    };
}

static inline PlanetState rk4_delta(PlanetState state, const SimulationState *simulation, usize index) {
    return (PlanetState) {
        .position = state.velocity,
        .velocity = compute_net_acceleration(state.position, simulation, index),
    };
}

// TODO: bucket or quadtree optimization?
// find way to delete planets for collision = MERGE
void collide_planets(SimulationState *simulation) {
    if (simulation->gui.collisions == NONE) return;

    for (usize i = 0; i < simulation->planets.length; i++) {
        for (usize j = i + 1; j < simulation->planets.length; j++) {
            Planet *a = &simulation->planets.data[i];
            Planet *b = &simulation->planets.data[j];
            
            f32 a_radius = planet_radius(simulation, a->mass);
            f32 b_radius = planet_radius(simulation, b->mass);

            Vector2 delta = Vector2Subtract(a->position, b->position);
            f32 distance = Vector2Length(delta);
            if (distance > a_radius + b_radius) continue;
            Vector2 relative_velocity = Vector2Subtract(a->velocity, b->velocity);
            if (Vector2DotProduct(delta, relative_velocity) > 0.0) continue;

            f32 angle = atan2f(delta.y, delta.x);
            Vector2 a_norm = Vector2Rotate(a->velocity, -angle);
            Vector2 b_norm = Vector2Rotate(b->velocity, -angle);
            f32 a_final = ((a->mass - b->mass) / (a->mass + b->mass)) * a_norm.x + ((2 * b->mass) / (a->mass + b->mass)) * b_norm.x;
            f32 b_final = ((2 * a->mass) / (a->mass + b->mass)) * a_norm.x + ((b->mass - a->mass) / (a->mass + b->mass)) * b_norm.x;
            a_norm.x = a_final;
            b_norm.x = b_final;

            a->velocity = Vector2Rotate(a_norm, angle);
            b->velocity = Vector2Rotate(b_norm, angle);
        }
    }
}

#endif
