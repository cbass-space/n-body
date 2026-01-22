#ifndef N_BODY_CONSTANTS
#define N_BODY_CONSTANTS

// application defaults
#define WIDTH_DEFAULT 1200
#define HEIGHT_DEFAULT 900
#define FIXED_DELTA_TIME_DEFAULT 0.01f
#define PREDICTION_DELTA_TIME_MULTIPLIER 1
#define EPSILON 1e-6f
#define MAX_ACCUMULATOR_TIME 0.25

// new body defaults
#define MASS_DEFAULT 50.0f
#define COLOR_DEFAULT (SDL_FColor) { 1.0f, 1.0f, 1.0f, 1.0f }

// simulation defaults
#define GRAVITY_DEFAULT 10000.0f
#define SOFTENING_DEFAULT 0.01f
#define DENSITY_DEFAULT 0.001f
#define INTEGRATOR_DEFAULT INTEGRATOR_VERLET
#define COLLISIONS_DEFAULT COLLISIONS_NONE

// graphics defaults
#define CLEAR_COLOR_DEFAULT (SDL_FColor) { 0.0f, 0.0f, 0.0f, 1.0f }
#define FIELD_GRID_DEFAULT 100.0f
#define MOVABLE_OUTLINE_DEFAULT 0.1f
#define STATIC_OUTLINE_DEFAULT 1.0f
#define TRAIL_FADE_DEFAULT 1.0f

// fixed (change is shaders as well)
#define TRAIL_LENGTH 512
#define PREDICTIONS_LENGTH 2048

#endif

