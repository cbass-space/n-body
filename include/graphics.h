#ifndef N_BODY_GRAPHICS
#define N_BODY_GRAPHICS

#include <stdbool.h>
#include "types.h"
#include "sdl_utils.h"

typedef struct Simulation Simulation;
typedef struct Trails Trails;
typedef struct Trajectories Trajectories;
typedef struct Camera Camera;

typedef struct {
    SDL_FColor clear_color;
    f32 movable_outline;
    f32 static_outline;
    f32 trail_brightness;
} GraphicsOptions;

typedef struct Graphics {
    GraphicsOptions options;
    u32 body_count;

    SDL_GPUGraphicsPipeline *body_pipeline;
    SDL_GPUGraphicsPipeline *trail_pipeline;
    SDL_GPUGraphicsPipeline *trajectory_pipeline;
    GPUArray colors;
} Graphics;

SDL_AppResult graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window);
typedef struct {
    SDL_GPUDevice *gpu;
    SDL_FColor color;
} GraphicsAddBodyInfo;

u32 graphics_add_body(Graphics *gfx, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, SDL_FColor *color);
typedef struct {
    SDL_Window *window;
    SDL_GPUDevice *gpu;
    const Simulation *sim;
    const Trails *trails;
    const Trajectories *trajectories;
    const Camera *cam;
} GraphicsDrawInfo;
void graphics_draw(const Graphics *gfx, SDL_GPUCommandBuffer *command_buffer, const GraphicsDrawInfo *info);
void graphics_free(const Graphics *gfx, SDL_GPUDevice *gpu);

#endif
