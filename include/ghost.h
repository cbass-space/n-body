#ifndef N_BODY_GHOST
#define N_BODY_GHOST

#include "SDL3/SDL.h"
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
    bool mode;
} Ghost;

void ghost_init(Ghost *ghost);
void ghost_update(Ghost *ghost, SDL_Window *window, const Simulation *sim, const Camera *cam);
bool ghost_mouse(Ghost *ghost, const SDL_Event *event);
void ghost_keyboard(Ghost *ghost, const SDL_Event *event);

#endif
