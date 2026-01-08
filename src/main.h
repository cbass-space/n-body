#ifndef N_BODY_MAIN
#define N_BODY_MAIN

#include "SDL3/SDL_gpu.h"
#include "dcimgui.h"
#include "HandmadeMath.h"

#include "types.h"
#include "sdl_utils.h"

// TODO: maybe make main.h only public fields?

// CONSTANTS //

// application defaults
#define WIDTH_DEFAULT 1200
#define HEIGHT_DEFAULT 900
#define FIXED_DELTA_TIME_DEFAULT 0.01f
#define TRAIL_LENGTH 256
#define EPSILON 1e-6f

// new body defaults
#define MASS_DEFAULT 50.0f
#define COLOR_DEFAULT (SDL_FColor) { 1.0f, 1.0f, 1.0f, 1.0f }

// simulation defaults
#define GRAVITY_DEFAULT 10000.0f
#define SOFTENING_DEFAULT 0.01f
#define DENSITY_DEFAULT 0.001f
#define INTEGRATOR_DEFAULT INTEGRATOR_VERLET
#define COLLISIONS_DEFAULT COLLISIONS_NONE
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
    bool barnes_hut;
} SimulationOptions;

// not sure about this chief
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

void simulation_init(Simulation *sim);
typedef struct {
    HMM_Vec2 position;
    HMM_Vec2 velocity;
    f32 mass;
    bool movable;
} SimulationAddBodyInfo;

usize simulation_add_body(Simulation *sim, const SimulationAddBodyInfo *body);
void simulation_update(Simulation *sim, f64 dt);
void simulation_free(Simulation *sim);
f32 body_radius(const Simulation *sim, f32 mass);

// CAMERA //
typedef struct {
    HMM_Vec2 position;
    usize target;
    f32 zoom;
} Camera;

void camera_init(Camera *cam);
void camera_mouse(Camera *cam, const SDL_Event *event, SDL_Window *window, bool ghost_mode);
void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim);
void camera_update(Camera *cam, const Simulation *sim);
HMM_Vec2 screen_to_world(const Camera *cam, SDL_Window *window, HMM_Vec2 position);
HMM_Vec2 world_to_screen(const Camera *cam, SDL_Window *window, HMM_Vec2 position);
HMM_Vec2 mouse_world_position(const Camera *cam, SDL_Window *window);

// CREATION GHOST BODY //

typedef struct {
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

// GRAPHICS //

typedef struct {
    enum {
        DIRTY_NONE,
        DIRTY_MASS,
        DIRTY_MOVABLE,
        DIRTY_COLOR,
    } type;
    usize index;
    union {
        f32 mass;
        bool movable;
        SDL_FColor color;
    };
} GraphicsDirtyFlag;

typedef struct {
    SDL_FColor clear_color;
    f32 movable_outline;
    f32 static_outline;
    f32 trail_brightness;
} GraphicsOptions;

typedef struct {
    GraphicsOptions options;
    GraphicsDirtyFlag dirty_flag;

    u32 trail_counter;
    u32 *offsets;
    SDL_FColor *colors;

    SDL_GPUGraphicsPipeline *circle_pipeline;
    SDL_GPUGraphicsPipeline *trail_pipeline;
    SDL_GPUGraphicsPipeline *ghost_pipeline;

    GPUArray gpu_trails;
    GPUArray gpu_offsets;
    GPUArray gpu_masses;
    GPUArray gpu_movables;
    GPUArray gpu_colors;
} Graphics;

i32 graphics_init(Graphics *gfx, SDL_GPUDevice *gpu, SDL_Window *window);
typedef struct {
    SDL_GPUDevice *gpu;
    SDL_FColor color;
    const Simulation *sim;
    usize index;
} GraphicsAddBodyInfo;
usize graphics_add_body(Graphics *gfx, GraphicsAddBodyInfo *info);
typedef struct {
    SDL_GPUDevice *gpu;
    SDL_Window *window;
    const Simulation *sim;
    const Ghost *ghost;
    const Camera *cam;
} GraphicsDrawInfo;
void graphics_draw(Graphics *gfx, const GraphicsDrawInfo *info);
void graphics_free(Graphics *gfx, SDL_GPUDevice *gpu);

// GUI //

typedef struct {
    f32 fixed_delta_time;
    bool paused;
} ApplicationOptions;

typedef struct {
    ImGuiIO *io;
} Gui;

i32 gui_init(Gui *gui, SDL_Window *window, SDL_GPUDevice *gpu);
typedef struct {
    ApplicationOptions *app;
    Simulation *sim;
    Graphics *gfx;
    Camera *cam;
    Ghost *ghost;
} GuiUpdateInfo;
void gui_update(const GuiUpdateInfo *info);
void gui_event(const SDL_Event *event);
void gui_free(void);

// APP //

typedef struct {
    ApplicationOptions options;
    SDL_Window *window;
    SDL_GPUDevice *gpu;
    Simulation sim;
    Ghost ghost;
    Camera cam;
    Graphics gfx;
    Gui gui;
} Application;

SDL_AppResult panic(const char *location, const char *message);

#endif
