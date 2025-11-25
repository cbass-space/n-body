#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"
#include "lib/types.h"
#include "lib/stb_ds.h"

#include "constants.h"

typedef struct {
    Vector2 position;
    Vector2 velocity;
    f32 mass;
    bool movable;
} Planet;

typedef enum {
    EULER,
    VERLET,
    RK4,
} Integrator;

typedef enum {
    NONE,
    MERGE,
    ELASTIC
} Collisions;

typedef struct {
    Integrator integrator;
    Collisions collisions;
    f32 gravity;
    f32 density;
    f32 softening;
    bool barnes_hut;
} SimulationParameters;

typedef struct {
    SimulationParameters parameters;
    Planet *planets;
} SimulationState;

void simulation_init(SimulationState *simulation);
void simulation_update(SimulationState *simulation, f32 delta_time);
f32 planet_radius(const SimulationState *simulation, f32 mass);

#ifdef SIM_IMPLEMENTATION

void simulation_init(SimulationState *simulation) {
    simulation->parameters = (SimulationParameters) {
        .gravity = GRAVITY_DEFAULT,
        .density = DENSITY_DEFAULT,
        .softening = SOFTENING_DEFAULT,
        .integrator = INTEGRATOR_DEFAULT,
        .collisions = COLLISIONS_DEFAULT,
        .barnes_hut = BARNES_HUT_DEFAULT
    };

    if (simulation->planets != NULL) {
        arrfree(simulation->planets);
        simulation->planets = NULL;
    }
}

void integrate_euler(SimulationState *simulation, usize index, f32 delta_time);
void integrate_verlet(SimulationState *simulation, usize index, f32 delta_time);
void integrate_rk4(SimulationState *simulation, usize index, f32 delta_time);
void collide_planets(SimulationState *simulation);

void simulation_update(SimulationState *simulation, f32 delta_time) {
    for (usize i = 0; i < arrlenu(simulation->planets); i++) {
        switch (simulation->parameters.integrator) {
            case EULER:
                integrate_euler(simulation, i, delta_time);
                break;
            case VERLET:
                integrate_verlet(simulation, i, delta_time);
                break;
            case RK4:
                integrate_rk4(simulation, i, delta_time);
                break;
        }
    }

    collide_planets(simulation);
}

Vector2 gravitational_acceleration(const SimulationState *simulation, Vector2 position, usize skip_index) {
    Vector2 net_acceleration = Vector2Zero();

    for (usize i = 0; i < arrlenu(simulation->planets); i++) {
        if (i == skip_index) continue;

        Planet *planet = &simulation->planets[i];
        Vector2 separation = Vector2Subtract(planet->position, position);
        f32 length_squared = Vector2LengthSqr(separation) + powf(simulation->parameters.softening, 2.0);
        if (length_squared < EPSILON) { return Vector2Zero(); }

        Vector2 acceleration = Vector2Scale(
            Vector2Normalize(separation),
            (simulation->parameters.gravity * planet->mass) / length_squared
        );

        net_acceleration = Vector2Add(net_acceleration, acceleration);
    }

    return net_acceleration;
}

// https://gafferongames.com/post/integration_basics/#semi-implicit-euler
void integrate_euler(SimulationState *simulation, usize planet_index, f32 delta_time) {
    Planet *planet = &simulation->planets[planet_index];
    Vector2 acceleration = gravitational_acceleration(simulation, planet->position, planet_index);
    planet->velocity = Vector2Add(planet->velocity, Vector2Scale(acceleration, delta_time));
    planet->position = Vector2Add(planet->position, Vector2Scale(planet->velocity, delta_time));
}

// https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
void integrate_verlet(SimulationState *simulation, usize planet_index, f32 delta_time) {
    Planet *planet = &simulation->planets[planet_index];

    Vector2 acceleration = gravitational_acceleration(simulation, planet->position, planet_index);
    planet->position = Vector2Add(planet->position, Vector2Add(
        Vector2Scale(planet->velocity, delta_time),
        Vector2Scale(acceleration, powf(delta_time, 2.0) / 2.0)
    ));

    Vector2 new_acceleration = gravitational_acceleration(simulation, planet->position, planet_index);
    planet->velocity = Vector2Add(planet->velocity,
        Vector2Scale(Vector2Add(acceleration, new_acceleration),
        delta_time / 2.0)
    );
}

typedef struct {
    Vector2 position;
    Vector2 velocity;
} KinematicState;
static inline KinematicState rk4_add(KinematicState a, KinematicState b);
static inline KinematicState rk4_scale(KinematicState state, f32 scale);
static inline KinematicState rk4_delta(KinematicState state, const SimulationState *simulation, usize planet_index);

void integrate_rk4(SimulationState *simulation, usize planet_index, f32 delta_time) {
    Planet *planet = &simulation->planets[planet_index];
    KinematicState state = { planet->position, planet->velocity };

    KinematicState k_1 = rk4_delta(state, simulation, planet_index);
    KinematicState k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, delta_time/2.0)), simulation, planet_index);
    KinematicState k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, delta_time/2.0)), simulation, planet_index);
    KinematicState k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, delta_time)), simulation, planet_index);

    KinematicState k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    KinematicState new_state = rk4_add(state, rk4_scale(k_sum, delta_time/6.0));
    planet->position = new_state.position;
    planet->velocity = new_state.velocity;
}

static inline KinematicState rk4_add(KinematicState a, KinematicState b) {
    return (KinematicState) {
        .position = Vector2Add(a.position, b.position),
        .velocity = Vector2Add(a.velocity, b.velocity),
    };
}

static inline KinematicState rk4_scale(KinematicState state, f32 scale) {
    return (KinematicState) {
        .position = Vector2Scale(state.position, scale),
        .velocity = Vector2Scale(state.velocity, scale),
    };
}

static inline KinematicState rk4_delta(KinematicState state, const SimulationState *simulation, usize planet_index) {
    return (KinematicState) {
        .position = state.velocity,
        .velocity = gravitational_acceleration(simulation, state.position, planet_index),
    };
}

void collide_elastic(SimulationState *simulation, usize planet_a_index, usize planet_b_index);
void collide_merge(SimulationState *simulation, usize planet_a_index, usize planet_b_index);

void collide_planets(SimulationState *simulation) {
    if (simulation->parameters.collisions == NONE) return;

    // TODO: bucket or quadtree optimization?
    for (usize i = 0; i < arrlenu(simulation->planets); i++) {
        for (usize j = i + 1; j < arrlenu(simulation->planets); j++) {
            Planet *a = &simulation->planets[i];
            Planet *b = &simulation->planets[j];
            f32 a_radius = planet_radius(simulation, a->mass);
            f32 b_radius = planet_radius(simulation, b->mass);

            Vector2 delta = Vector2Subtract(a->position, b->position);
            f32 distance = Vector2Length(delta);
            if (distance > a_radius + b_radius) continue;
            Vector2 relative_velocity = Vector2Subtract(a->velocity, b->velocity);
            if (Vector2DotProduct(delta, relative_velocity) > 0.0) continue;

            if (simulation->parameters.collisions == ELASTIC) collide_elastic(simulation, i, j);
            if (simulation->parameters.collisions == MERGE) collide_merge(simulation, i, j);
        }
    }
}

// https://www.youtube.com/watch?v=bSVfItpvG5Q & https://en.wikipedia.org/wiki/Elastic_collision
void collide_elastic(SimulationState *simulation, usize planet_a_index, usize planet_b_index) {
    Planet *a = &simulation->planets[planet_a_index];
    Planet *b = &simulation->planets[planet_b_index];
    f32 a_radius = planet_radius(simulation, a->mass);
    f32 b_radius = planet_radius(simulation, b->mass);
    Vector2 delta = Vector2Subtract(a->position, b->position);
    f32 distance = Vector2Length(delta);

    // TODO: coefficient of restitution?
    f32 angle = atan2f(delta.y, delta.x);
    Vector2 a_norm = Vector2Rotate(a->velocity, -angle);
    Vector2 b_norm = Vector2Rotate(b->velocity, -angle);
    f32 a_final = ((a->mass - b->mass) / (a->mass + b->mass)) * a_norm.x + ((2 * b->mass) / (a->mass + b->mass)) * b_norm.x;
    f32 b_final = ((2 * a->mass) / (a->mass + b->mass)) * a_norm.x + ((b->mass - a->mass) / (a->mass + b->mass)) * b_norm.x;
    a_norm.x = a_final;
    b_norm.x = b_final;

    a->velocity = Vector2Rotate(a_norm, angle);
    b->velocity = Vector2Rotate(b_norm, angle);

    f32 overlap_distance = a_radius + b_radius - distance;
    if (overlap_distance > EPSILON) {
        Vector2 overlap = Vector2Scale(Vector2Normalize(delta), a_radius + b_radius - distance);
        a->position = Vector2Add(a->position, Vector2Scale(overlap, 1.0/2.0));
        b->position = Vector2Subtract(b->position, Vector2Scale(overlap, 1.0/2.0));
    }
}

void collide_merge(SimulationState *simulation, usize planet_a_index, usize planet_b_index) {
    Planet *a = &simulation->planets[planet_a_index];
    Planet *b = &simulation->planets[planet_b_index];
    a->mass = a->mass + b->mass;
    a->velocity = Vector2Scale(Vector2Add(
        Vector2Scale(a->velocity, a->mass),
        Vector2Scale(b->velocity, b->mass)
    ), 1.0 / (a->mass + b->mass));

    // FIXME: fix with generational indices?
    arrdel(simulation->planets, planet_b_index);
}

f32 planet_radius(const SimulationState *simulation, f32 mass) {
    return powf(mass / simulation->parameters.density, 1.0/3.0);
}

#endif

