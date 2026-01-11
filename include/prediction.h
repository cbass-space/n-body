#ifndef N_BODY_PREDICTION
#define N_BODY_PREDICTION

#include "HandmadeMath.h"
#include "types.h"

typedef struct Simulation Simulation;
typedef struct Ghost Ghost;

typedef struct Predictions {
    HMM_Vec2 *positions;
    HMM_Vec2 *ghost_positions;
} Predictions;

void prediction_update(Predictions *predictions, const Simulation *sim, const Ghost *ghost, f32 delta_time);
void prediction_free(Predictions *predictions);

#endif
