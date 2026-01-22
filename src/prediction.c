#include "prediction.h"
#include "constants.h"
#include "simulation.h"
#include "ghost.h"

#include "stb_ds.h"

void prediction_init(Predictions *predictions) {
    predictions->enabled = true;
}

void prediction_update(Predictions *predictions, const Simulation *sim, const Ghost *ghost, const f32 delta_time) {
    if (!predictions->enabled) return;

    Simulation sim_copy = { 0 };
    simulation_copy(sim, &sim_copy);
    sim_copy.options.paused = false;
    if (ghost->enabled) simulation_add_body(&sim_copy, &(SimulationAddBodyInfo) {
        .position = ghost->position,
        .velocity = ghost->velocity,
        .mass = ghost->mass,
        .movable = ghost->movable
    });

    arrsetlen(predictions->positions, PREDICTIONS_LENGTH * sim->body_count);
    if (ghost->enabled) arrsetlen(predictions->ghost_positions, PREDICTIONS_LENGTH);
    for (usize i = 0; i < PREDICTIONS_LENGTH; i++) {
        simulation_update(&sim_copy, delta_time);
        for (usize j = 0; j < sim->body_count; j++) predictions->positions[j * PREDICTIONS_LENGTH + i] = sim_copy.positions[j];
        if (ghost->enabled) predictions->ghost_positions[i] = sim_copy.positions[arrlenu(sim_copy.positions) - 1];
    }
}

void prediction_free(Predictions *predictions) {
    arrfree(predictions->positions);
    arrfree(predictions->ghost_positions);
}

