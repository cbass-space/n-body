#pragma once

#include "lib/types.h"

#include "constants.h"
#include "simulation.h"
#include "camera.h"
#include "predictor.h"

typedef struct {
    Vector2 positions[TRAIL_MAX];
    usize oldest;
    usize count;
} Trail;

typedef struct {
    Trail trail;
    Color color;
} Visual;

typedef struct {
    f32 trail;
    bool draw_relative;
    bool draw_field_grid;
    bool draw_velocity;
    bool draw_forces;
    bool draw_net_force;
} DrawParameters;

typedef struct {
    DrawParameters parameters;
    Visual *visuals;
} DrawState;

void draw_init(DrawState *drawer);
void draw(const DrawState *drawer, const SimulationState *simulation, const PredictorState *predictor, const CameraState *camera);

#ifdef DRAW_IMPLEMENTATION

void DrawVector(Vector2 start, Vector2 vector, Color color);
static inline usize trail_nth_latest(Trail* trail, usize n);

void draw_init(DrawState *drawer) {
    drawer->parameters = (DrawParameters) {
        .trail = TRAIL_DEFAULT,
        .draw_relative = false,
        .draw_field_grid = false,
        .draw_velocity = false,
        .draw_forces = false,
        .draw_net_force = false
    };

    if (drawer->visuals != NULL) {
        arrfree(drawer->visuals);
        drawer->visuals = NULL;
    }
}

void draw(const DrawState *drawer, const SimulationState *simulation, const PredictorState *predictor, const CameraState *camera) {
    for (usize i = 0; i < arrlenu(simulation->planets); i++) {
        Planet *planet = &simulation->planets[i];
        Visual *visual = &drawer->visuals[i];

        visual->trail.positions[trail_nth_latest(&visual->trail, 0)] = planet->position;
        if (visual->trail.count < TRAIL_MAX) visual->trail.count++; 
        else visual->trail.oldest = (visual->trail.oldest + 1) % TRAIL_MAX;
    }

    for (usize i = 0; i < arrlenu(simulation->planets); i++) {
        Planet *planet = &simulation->planets[i];
        Visual *visual = &drawer->visuals[i];

        void (*draw_planet)(Vector2, f32, Color) = planet->movable ? DrawCircleLinesV : DrawCircleV;
        draw_planet(
            planet->position,
            planet_radius(simulation, planet->mass),
            visual->color
        );

        // TODO: draw_forces, draw_field_grid
        
        if (!planet->movable && camera->target == (usize) -1) continue;
        usize trail_count = visual->trail.count < drawer->parameters.trail ? visual->trail.count : drawer->parameters.trail;
        for (usize i = 1; i < trail_count; i++) {
            Vector2 point_a = visual->trail.positions[trail_nth_latest(&visual->trail, i)];
            Vector2 point_b = visual->trail.positions[trail_nth_latest(&visual->trail, i + 1)];

            if (drawer->parameters.draw_relative && camera->target != (usize) -1) {
                Planet *target = &simulation->planets[camera->target];
                Visual *target_visual = &drawer->visuals[camera->target];
                point_a = Vector2Add(target->position, Vector2Subtract(point_a, target_visual->trail.positions[trail_nth_latest(&target_visual->trail, i)]));
                point_b = Vector2Add(target->position, Vector2Subtract(point_b, target_visual->trail.positions[trail_nth_latest(&target_visual->trail, i + 1)]));
            }

            Color color = visual->color;
            color.a = 256 * (trail_count - i) / trail_count;
            DrawLineV(point_a, point_b, color);
        }

        if (drawer->parameters.draw_velocity) {
            Vector2 target_velocity = (camera->target != (usize) -1 && drawer->parameters.draw_relative)
                ? simulation->planets[camera->target].velocity
                : Vector2Zero();
            DrawVector(planet->position, Vector2Subtract(planet->velocity, target_velocity), visual->color);
        }
    }

    for (usize i = 0; i < arrlenu(predictor->predictions); i++) {
        Visual *visual = &drawer->visuals[i];

        for (usize step = 1; step < PREDICT_LENGTH; step++) {
            Vector2 point_a = predictor->predictions[i].positions[step];
            Vector2 point_b = predictor->predictions[i].positions[step - 1];

            if (drawer->parameters.draw_relative && camera->target != (usize) -1) {
                Planet *target = &simulation->planets[camera->target];
                point_a = Vector2Add(target->position, Vector2Subtract(point_a, predictor->predictions[camera->target].positions[step]));
                point_b = Vector2Add(target->position, Vector2Subtract(point_b, predictor->predictions[camera->target].positions[step - 1]));
            }

            Color color = visual->color;
            color.a = 256 * (PREDICT_LENGTH - step) / (3 * PREDICT_LENGTH);
            DrawLineV(point_a, point_b, color);
        }
    }
}

static inline usize trail_nth_latest(Trail* trail, usize n) {
    return (trail->oldest + trail->count - n) % TRAIL_MAX;
}

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

#endif

