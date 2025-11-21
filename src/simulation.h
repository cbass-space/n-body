#pragma once

#include "raylib.h"
#include "raymath.h"
#include "lib/types.h"
#include "lib/stb_ds.h"

#include "constants.h"
#include <stdio.h>

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
    f32 gravity;
    f32 density;
    f32 softening;
    Integrator integrator;
    Collisions collisions;
    bool barnes_hut;
} SimulationParameters;

typedef struct {
    Vector2 positions[PREDICT_LENGTH];
    Vector2 scratch_velocity; // TODO: might not need
} Prediction;

typedef struct {
    Vector2 positions[TRAIL_MAX];
    usize count;
    usize oldest;
} Trail;

typedef struct {
    Vector2 position; // TODO: replace with PlanetState?
    Vector2 velocity;
    Prediction prediction;
    Trail trail;

    f32 mass;
    bool movable;
    Color color;
} Planet;

typedef struct {
    SimulationParameters parameters;
    Planet *planets;
    Planet creation; // TODO: move to outside of Simulation?
} Simulation;

void simulation_init(Simulation *simulation);
void simulation_update(Simulation *simulation, f32 time_step);
f32 planet_radius(const Simulation *simulation, f32 mass);
usize trail_nth_latest(const Trail *trail, usize n);

Vector2 compute_acceleration(const Simulation *simulation, Vector2 position, usize planet_index, isize prediction_index);
Vector2 compute_net_acceleration(const Simulation *simulation, Vector2 position, isize planet_index, isize prediction_index);

#ifdef SIM_IMPLEMENTATION

void simulation_init(Simulation *simulation) {
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

typedef struct {
    Vector2 position;
    Vector2 velocity;
} PlanetState;

const PlanetState euler_update(const Simulation *simulation, f32 time_step, usize planet_index, isize prediction_index);
const PlanetState verlet_update(const Simulation *simulation, f32 time_step, usize planet_index, isize prediction_index);
const PlanetState rk4_update(const Simulation *simulation, f32 time_step, usize planet_index, isize prediction_index);
static const PlanetState (*integrators[])(const Simulation*, f32, usize, isize) = { euler_update, verlet_update, rk4_update };

void collide_planets(Simulation *simulation);
void predict_trajectories(Simulation *simulation, f32 time_step);
void simulation_update(Simulation *simulation, f32 time_step) {
    for (usize i = 0; i < arrlen(simulation->planets); i++) {
        Planet *planet = &simulation->planets[i];
        if (!planet->movable) continue;

        PlanetState new_state = integrators[simulation->parameters.integrator](simulation, time_step, i, -1);
        planet->position = new_state.position;
        planet->velocity = new_state.velocity;

        planet->trail.positions[trail_nth_latest(&planet->trail, 0)] = planet->position;
        if (planet->trail.count < TRAIL_MAX) {
            planet->trail.count++;
        } else {
            planet->trail.oldest = (planet->trail.oldest + 1) % TRAIL_MAX;
        }
    }

    collide_planets(simulation);
    predict_trajectories(simulation, time_step);
}

Vector2 compute_acceleration(const Simulation *simulation, Vector2 position, usize planet_index, isize prediction_index) {
    Planet *planet = &simulation->planets[planet_index];
    Vector2 planet_position = (prediction_index == -1)
        ? planet->position
        : planet->prediction.positions[prediction_index];

    Vector2 delta = Vector2Subtract(planet_position, position);
    f32 length_squared = Vector2LengthSqr(delta) + powf(simulation->parameters.softening, 2.0);
    if (length_squared < EPSILON) { return Vector2Zero(); }

    Vector2 direction = Vector2Normalize(delta);
    return Vector2Scale(direction, (simulation->parameters.gravity * planet->mass) / length_squared);
}

// TODO: barnes hut? 
Vector2 compute_net_acceleration(const Simulation *simulation, Vector2 position, isize planet_index, isize prediction_index) {
    Vector2 net_acceleration = Vector2Zero();
    for (usize i = 0; i < arrlen(simulation->planets); i++) {
        if (i == planet_index) { continue; }
        Vector2 acceleration = compute_acceleration(simulation, position, i, prediction_index);
        net_acceleration = Vector2Add(net_acceleration, acceleration);
    }

    return net_acceleration;
}

// https://gafferongames.com/post/integration_basics/#semi-implicit-euler
const PlanetState euler_update(const Simulation *simulation, f32 time_step, usize planet_index, isize prediction_index) {
    Planet *planet = &simulation->planets[planet_index];
    Vector2 planet_position = (prediction_index == -1)
        ? planet->position
        : planet->prediction.positions[prediction_index];
    Vector2 planet_velocity = (prediction_index == -1)
        ? planet->velocity
        : planet->prediction.scratch_velocity; // TODO: so ugly...

    Vector2 acceleration = compute_net_acceleration(simulation, planet_position, planet_index, prediction_index);
    Vector2 velocity_new = Vector2Add(planet_velocity, Vector2Scale(acceleration, time_step));
    Vector2 position_new = Vector2Add(planet_position, Vector2Scale(velocity_new, time_step));

    return (PlanetState) {
        .velocity = velocity_new,
        .position = position_new
    };
}

// https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
const PlanetState verlet_update(const Simulation *simulation, f32 time_step, usize planet_index, isize prediction_index) {
    Planet *planet = &simulation->planets[planet_index];
    Vector2 planet_position = (prediction_index == -1)
        ? planet->position
        : planet->prediction.positions[prediction_index];
    Vector2 planet_velocity = (prediction_index == -1)
        ? planet->velocity
        : planet->prediction.scratch_velocity;

    Vector2 acceleration = compute_net_acceleration(simulation, planet_position, planet_index, prediction_index);
    Vector2 position_new = Vector2Add(
        Vector2Add(planet_position, Vector2Scale(planet_velocity, time_step)),
        Vector2Scale(acceleration, powf(time_step, 2.0) / 2.0)
    );

    Vector2 acceleration_new = compute_net_acceleration(simulation, position_new, planet_index, prediction_index);
    Vector2 velocity_new = Vector2Add(
        planet_velocity,
        Vector2Scale(Vector2Add(acceleration, acceleration_new), time_step / 2.0)
    );

    return (PlanetState){
        .position = position_new,
        .velocity = velocity_new
    };
}

PlanetState rk4_add(PlanetState a, PlanetState b);
PlanetState rk4_scale(PlanetState state, f32 scale);
PlanetState rk4_delta(PlanetState state, const Simulation *simulation, usize planet_index, isize prediction_index);

// https://en.wikipedia.org/wiki/Runge-Kutta_methods
const PlanetState rk4_update(const Simulation *simulation, f32 time_step, usize planet_index, isize prediction_index) {
    Planet *planet = &simulation->planets[planet_index];
    Vector2 planet_position = (prediction_index == -1)
        ? planet->position
        : planet->prediction.positions[prediction_index];
    Vector2 planet_velocity = (prediction_index == -1)
        ? planet->velocity
        : planet->prediction.scratch_velocity;

    PlanetState state = {
        .position = planet_position,
        .velocity = planet_velocity
    };

    PlanetState k_1 = rk4_delta(state, simulation, planet_index, prediction_index);
    PlanetState k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, time_step/2.0)), simulation, planet_index, prediction_index);
    PlanetState k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, time_step/2.0)), simulation, planet_index, prediction_index);
    PlanetState k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, time_step)), simulation, planet_index, prediction_index);

    PlanetState k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    return rk4_add(state, rk4_scale(k_sum, time_step/6.0));
}

PlanetState rk4_add(PlanetState a, PlanetState b) {
    return (PlanetState) {
        .position = Vector2Add(a.position, b.position),
        .velocity = Vector2Add(a.velocity, b.velocity),
    };
}

PlanetState rk4_scale(PlanetState state, f32 scale) {
    return (PlanetState) {
        .position = Vector2Scale(state.position, scale),
        .velocity = Vector2Scale(state.velocity, scale),
    };
}

PlanetState rk4_delta(PlanetState state, const Simulation *simulation, usize planet_index, isize prediction_index) {
    return (PlanetState) {
        .position = state.velocity,
        .velocity = compute_net_acceleration(simulation, state.position, planet_index, prediction_index),
    };
}

// TODO: bucket or quadtree optimization?
// TODO: add positional correction for intersecting planets in elastic collision
void collide_planets(Simulation *simulation) {
    if (simulation->parameters.collisions == NONE) return;

    for (usize i = 0; i < arrlen(simulation->planets); i++) {
        for (usize j = i + 1; j < arrlen(simulation->planets); j++) {
            Planet *a = &simulation->planets[i];
            Planet *b = &simulation->planets[j];
            f32 a_radius = planet_radius(simulation, a->mass);
            f32 b_radius = planet_radius(simulation, b->mass);

            Vector2 delta = Vector2Subtract(a->position, b->position);
            f32 distance = Vector2Length(delta);
            if (distance > a_radius + b_radius) continue;

            if (simulation->parameters.collisions == MERGE) {
                a->mass = a->mass + b->mass;
                a->velocity = Vector2Scale(Vector2Add(
                    Vector2Scale(a->velocity, a->mass),
                    Vector2Scale(b->velocity, b->mass)
                ), 1.0 / (a->mass + b->mass));

                // FIXME: might cause problems later...
                arrdel(simulation->planets, j);
            }

            if (simulation->parameters.collisions == ELASTIC) {
                // TODO: coefficient of restitution?
                // https://www.youtube.com/watch?v=bSVfItpvG5Q & https://en.wikipedia.org/wiki/Elastic_collision
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
                
                f32 overlap_distance = a_radius + b_radius - distance;
                if (overlap_distance > EPSILON) {
                    Vector2 overlap = Vector2Scale(Vector2Normalize(delta), a_radius + b_radius - distance);
                    a->position = Vector2Add(a->position, Vector2Scale(overlap, 1.0/2.0));
                    b->position = Vector2Subtract(b->position, Vector2Scale(overlap, 1.0/2.0));
                }
            }
        }
    }
}

void predict_trajectories(Simulation *simulation, f32 time_step) {
    for (usize i = 0; i < arrlen(simulation->planets); i++) {
        Planet *planet = &simulation->planets[i];
        planet->prediction.positions[0] = planet->position;
        planet->prediction.scratch_velocity = planet->velocity;
    }

    for (usize step = 1; step < PREDICT_LENGTH; step++) {
        for (usize i = 0; i < arrlen(simulation->planets); i++) {
            Planet *planet = &simulation->planets[i];
            PlanetState state = integrators[simulation->parameters.integrator](simulation, time_step, i, step - 1);
            planet->prediction.scratch_velocity = state.velocity;
            planet->prediction.positions[step] = state.position;
        }
    }
}

f32 planet_radius(const Simulation *simulation, f32 mass) {
    return powf(mass / simulation->parameters.density, 1.0/3.0);
}

usize trail_nth_latest(const Trail *trail, usize i) {
    return (trail->oldest + trail->count - i) % TRAIL_MAX;
}

#endif
