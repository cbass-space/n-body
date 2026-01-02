#ifndef N_BODY_MAIN
#define N_BODY_MAIN

#include "HandmadeMath.h"

#include "SDL3/SDL_gpu.h"
#include "dcimgui.h"

#include "types.h"
#include "sdl_utils.h"

// CONSTANTS //

// application defaults
#define WIDTH_DEFAULT 1200
#define HEIGHT_DEFAULT 900
#define FIXED_DELTA_TIME_DEFAULT 0.01f // add slider?
#define TRAIL_LENGTH 256
#define EPSILON 1e-6f

// body defaults
#define MASS_DEFAULT 50.0f
#define COLOR_DEFAULT (SDL_FColor) { 1.0f, 1.0f, 1.0f, 1.0f }

// simulation defaults
#define GRAVITY_DEFAULT 10000.0f
#define SOFTENING_DEFAULT 0.01f
#define DENSITY_DEFAULT 0.001f
#define INTEGRATOR_DEFAULT EULER
#define COLLISIONS_DEFAULT NONE
#define BARNES_HUT_DEFAULT false

// graphics defaults
#define CLEAR_COLOR_DEFAULT (SDL_FColor) { 0.0f, 0.0f, 0.0f, 1.0f }
#define FIELD_GRID_DEFAULT 100.0f
#define MOVABLE_OUTLINE_DEFAULT 0.1f
#define STATIC_OUTLINE_DEFAULT 1.0f
#define PREDICT_LENGTH_DEFAULT 4096
#define PREDICT_LENGTH_MAX 8192
#define TRAIL_FADE_DEFAULT 1.0f

// SIMULATION //

typedef struct {
    HMM_Vec2 position;
    HMM_Vec2 velocity;
    f32 mass;
    bool movable;
    SDL_FColor color;
} NewBody;

typedef enum {
    EULER,
    VERLET,
    RK4,
} Integrator;

typedef enum {
    NONE,
    MERGE,
    BOUNCE
} Collisions;

typedef struct {
    Integrator integrator;
    Collisions collisions;
    f32 gravity;
    f32 softening;
    f32 density;
    bool barnes_hut;
} SimulationOptions;

typedef struct {
    SimulationOptions options;
    /// body positions
    HMM_Vec2 *r;
    /// body velocities
    HMM_Vec2 *v;
    /// body masses
    f32 *m;
    /// whether to simulate or remain static
    bool *movable;
} Simulation;

i32 simulation_init(Simulation *sim);
void simulation_add_body(Simulation *sim, const NewBody *body);
void simulation_update(Simulation *sim, f64 dt);
void simulation_free(Simulation *sim);
f32 body_radius(const Simulation *sim, f32 mass);

// CAMERA //
typedef struct {
    HMM_Vec2 position;
    f32 zoom;
    usize target;
} Camera;

void camera_init(Camera *cam);
void camera_mouse(Camera *cam, const SDL_Event *event, SDL_Window *window);
void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim);
void camera_update(Camera *cam, const Simulation *sim);

HMM_Vec2 screen_to_world(const Camera *camera, SDL_Window *window, HMM_Vec2 position);
HMM_Vec2 world_to_screen(const Camera *camera, SDL_Window *window, HMM_Vec2 position);
HMM_Vec2 mouse_world_position(const Camera *camera, SDL_Window *window);

// GRAPHICS //

typedef struct {
    SDL_FColor clear_color;
    f32 movable_outline;
    f32 static_outline;
    f32 trail_brightness;
} GraphicsOptions;

typedef struct {
    GraphicsOptions options;

    SDL_GPUCommandBuffer *command_buffer;
    SDL_GPUTexture *swapchain;

    GPUArray trail_positions;
    GPUArray trail_offsets;
    GPUArray colors;
    GPUArray masses;
    u32 *offsets;
    u32 trail_counter;

    SDL_GPUGraphicsPipeline *circle_pipeline;
    SDL_GPUGraphicsPipeline *trail_pipeline;
} Graphics;

i32 graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window);
void graphics_push_body(Graphics *gfx, SDL_GPUDevice *gpu, const NewBody *body);
void graphics_draw(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window, const Simulation *sim, const Camera *camera);
void graphics_free(Graphics *gfx, SDL_GPUDevice *gpu);

// GUI //

typedef struct {
    f32 fixed_delta_time;
    bool paused;

    bool create_mode;
    NewBody create_body;
} ApplicationOptions;

typedef struct {
    ImGuiIO *io;
} Gui;

i32 gui_init(Gui *gui, SDL_Window *window, SDL_GPUDevice *gpu);
void gui_update(ApplicationOptions *app, Simulation *sim, Graphics *gfx, Camera *cam);
void gui_event(const SDL_Event *event);
void gui_draw(const Gui *gui, const Graphics *gfx);
void gui_free(void);

// APP //

typedef struct {
    ApplicationOptions options;

    SDL_Window *window;
    SDL_GPUDevice *gpu;
    Simulation sim;
    Camera camera;
    Graphics graphics;
    Gui gui;
} Application;

i32 panic(const char *location, const char *message);

i32 app_init(Application *app);
i32 app_panic(Application *app, const char *location, const char *message);
void app_update(Application *app);
void app_fixed_update(Application *app, f64 delta_time);
void app_event(Application *app, const SDL_Event *event);
void app_draw(Application *app);
void app_free(Application *app);

#endif
