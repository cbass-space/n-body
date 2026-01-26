#ifndef N_BODY_GRAPHICS
#define N_BODY_GRAPHICS

#include <stdbool.h>
#include "SDL3/SDL.h"
#include "types.h"
#include "sdl_utils.h"

typedef struct Simulation Simulation;
typedef struct Trails Trails;
typedef struct Trajectories Trajectories;
// typedef struct Camera Camera;
// typedef struct Ghost Ghost;

// typedef struct {
//     enum {
//         DIRTY_NONE,
//         DIRTY_MASS,
//         DIRTY_MOVABLE,
//         DIRTY_COLOR,
//     } type;
//     usize index;
//     union {
//         f32 mass;
//         bool movable;
//         SDL_FColor color;
//     };
// } GraphicsDirtyFlag;

typedef struct {
    SDL_FColor clear_color;
    f32 movable_outline;
    f32 static_outline;
    f32 trail_brightness;
} GraphicsOptions;

typedef struct Graphics {
    GraphicsOptions options;
    // GraphicsDirtyFlag dirty_flag;

    // u32 trail_counter;
    u32 body_count;

    SDL_GPUGraphicsPipeline *body_pipeline;
    SDL_GPUGraphicsPipeline *trail_pipeline;
    SDL_GPUGraphicsPipeline *trajectory_pipeline;
    // SDL_GPUGraphicsPipeline *gpu_ghost_circle_pipeline;
    // SDL_GPUGraphicsPipeline *gpu_ghost_prediction_pipeline;

    GPUArray colors;
    // SDL_GPUBuffer *gpu_ghost_predictions;
} Graphics;

i32 graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window);
typedef struct {
    SDL_GPUDevice *gpu;
    SDL_FColor color;
} GraphicsAddBodyInfo;

u32 graphics_add_body(Graphics *gfx, SDL_GPUDevice *gpu, SDL_FColor *color);
typedef struct {
    SDL_GPUDevice *gpu;
    SDL_Window *window;
    const Simulation *sim;
    const Trails *trails;
    const Trajectories *trajectories;
    // const Camera *cam;
    // const Ghost *ghost;
} GraphicsDrawInfo;
void graphics_draw(const Graphics *gfx, const GraphicsDrawInfo *info);
void graphics_free(const Graphics *gfx, SDL_GPUDevice *gpu);

#endif
