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
    enum {
        COLLISIONS_NONE,
        COLLISIONS_MERGE,
        COLLISIONS_BOUNCE
    } collisions;
    f32 gravity;
    f32 softening;
    f32 density;
    bool paused;
} SimulationOptions;

// not sure about this chief
typedef struct Simulation {
    SimulationOptions options;
    // body positions
    HMM_Vec2 *r;
    // body velocities
    HMM_Vec2 *v;
    // body masses
    f32 *m;
    // whether to simulate or remain static
    bool *movable;
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
