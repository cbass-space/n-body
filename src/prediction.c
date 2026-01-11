#include "prediction.h"
#include "constants.h"
#include "simulation.h"
#include "ghost.h"

#include "stb_ds.h"

void prediction_update(Predictions *predictions, const Simulation *sim, const Ghost *ghost, const f32 delta_time) {
    Simulation sim_copy = { 0 };
    simulation_copy(sim, &sim_copy);
    if (ghost->mode) simulation_add_body(&sim_copy, &(SimulationAddBodyInfo) {
        .position = ghost->position,
        .velocity = ghost->velocity,
        .mass = ghost->mass,
        .movable = ghost->movable
    });

    if (ghost->mode) arrsetlen(predictions->ghost_positions, PREDICTIONS_LENGTH);
    arrsetlen(predictions->positions, PREDICTIONS_LENGTH * arrlen(sim->r));
    for (usize i = 0; i < PREDICTIONS_LENGTH; i++) {
        simulation_update(&sim_copy, delta_time);
        for (usize j = 0; j < arrlenu(sim->r); j++) predictions->positions[j * PREDICTIONS_LENGTH + i] = sim_copy.r[j];
        if (ghost->mode) predictions->ghost_positions[i] = sim_copy.r[arrlenu(sim_copy.r) - 1];
    }
}

void prediction_free(Predictions *predictions) {
    arrfree(predictions->positions);
    arrfree(predictions->ghost_positions);
}

