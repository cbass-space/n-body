#pragma once

#include "lib/types.h"
#include "raylib.h"

#include "constants.h"
#include "simulation.h"
#include <stdio.h>

typedef struct {
    Vector2 positions[PREDICT_LENGTH];
} Prediction;

typedef struct {
    Prediction *predictions;
} PredictorState;

void predictor_init(PredictorState *predictor);
void predictor_update(PredictorState *predictor, SimulationState *simulation, f32 delta_time);

#ifdef PREDICTOR_IMPLEMENTATION

void predictor_init(PredictorState *predictor) {
    if (predictor->predictions != NULL) {
        arrfree(predictor->predictions);
        predictor->predictions = NULL;
    }
}

void predictor_update(PredictorState *predictor, SimulationState *simulation, f32 delta_time) {
    SimulationState simulation_copy = (SimulationState) {
        .parameters = simulation->parameters,
        .planets = NULL
    };

    arrsetcap(simulation_copy.planets, arrlen(simulation->planets));
    for (usize i = 0; i < arrlenu(simulation->planets); i++) {
        arrpush(simulation_copy.planets, simulation->planets[i]);
    }

    for (usize step = 0; step < PREDICT_LENGTH; step++) {
        simulation_update(&simulation_copy, delta_time);

        for (usize i = 0; i < arrlenu(predictor->predictions); i++) {
            predictor->predictions[i].positions[step] = simulation_copy.planets[i].position;
        }
    }

    arrfree(simulation_copy.planets);
}

#endif

