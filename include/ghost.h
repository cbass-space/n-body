#ifndef N_BODY_GHOST
#define N_BODY_GHOST

#include <stdbool.h>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_pixels.h"
#include "HandmadeMath.h"
#include "types.h"

typedef struct Simulation Simulation;
typedef struct Camera Camera;

typedef struct Ghost {
    SDL_FColor color;
    HMM_Vec2 position;
    HMM_Vec2 velocity;
    HMM_Vec2 relative_position;
    f32 mass;
    bool movable;
    bool enabled;
} Ghost;

void ghost_init(Ghost *ghost);
void ghost_update(Ghost *ghost, SDL_GPUDevice *gpu, const Simulation *sim, const Camera *cam);
bool ghost_mouse(Ghost *ghost, const SDL_Event *event);
void ghost_keyboard(Ghost *ghost, const SDL_Event *event);

#endif

