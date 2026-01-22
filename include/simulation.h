#ifndef N_BODY_SIMULATION
#define N_BODY_SIMULATION

#include <stdbool.h>
#include "HandmadeMath.h"
#include "types.h"

typedef struct {
    enum {
        INTEGRATOR_EULER,
        INTEGRATOR_VERLET,
        INTEGRATOR_RK4,
    } integrator;
    f32 gravity;
    f32 softening;
    f32 density;
    bool collide;
    bool paused;
} SimulationOptions;

typedef struct Simulation {
    SimulationOptions options;
    HMM_Vec2 *positions;
    HMM_Vec2 *velocities;
    f32 *masses;
    bool *movable;
    u32 body_count;
} Simulation;

void simulation_init(Simulation *sim);
typedef struct {
    HMM_Vec2 position;
    HMM_Vec2 velocity;
    f32 mass;
    bool movable;
} SimulationAddBodyInfo;
usize simulation_add_body(Simulation *sim, const SimulationAddBodyInfo *body);
void simulation_update(Simulation *sim, f64 dt);
void simulation_copy(const Simulation *source, Simulation *destination);
void simulation_free(Simulation *sim);
f32 body_radius(const Simulation *sim, f32 mass);

#endif
