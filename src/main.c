#include "SDL3/SDL_video.h"
#include "constants.h"
#include "simulation.h"
#include "trails.h"
#include "trajectory.h"
#include "camera.h"
#include "graphics.h"
#include "gui.h"

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_gpu.h"
#include "types.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define UNUSED(x) (void)(x)

typedef struct {
    ApplicationOptions options;
    SDL_Window *window;
    SDL_GPUDevice *gpu;

    Simulation sim;
    Trails trails;
    Trajectories trajectories;
    Camera cam;
    Graphics gfx;
    Gui gui;
} Application;

static void add_body(Application *app, SDL_GPUCopyPass *copy_pass, const SimulationAddBodyInfo *sim_info, SDL_FColor *color);
SDL_AppResult SDL_AppInit(void **appstate, const int argc, char **argv) {
    UNUSED(argc); UNUSED(argv);
    Application *app = SDL_calloc(1, sizeof(*app));
    *appstate = app;
    app->options = (ApplicationOptions) { .fixed_delta_time = FIXED_DELTA_TIME_DEFAULT };

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) panic("Failed to initialize SDL3!");
    const f32 main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    app->window = SDL_CreateWindow("N-Body Simulation", (i32)(WIDTH_DEFAULT * main_scale), (i32)(HEIGHT_DEFAULT * main_scale), window_flags);
    if (!app->window) panic("Failed to create SDL window!");
    SDL_ShowWindow(app->window);

    SDL_SetLogPriority(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_VERBOSE);

    app->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (!app->gpu) panic("Failed to create GPU device!");
    if (!SDL_ClaimWindowForGPUDevice(app->gpu, app->window)) panic("Failed to claim window for GPU!");
    SDL_SetGPUSwapchainParameters(app->gpu, app->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // initialize modules
    if (simulation_init(&app->sim, app->gpu) != 0) panic("Failed to initialize simulation!");
    if (trails_init(&app->trails, app->gpu) != 0) panic("Failed to initialize trail module!");
    if (trajectories_init(&app->trajectories, app->gpu) != 0) panic("Failed to initialize trajectory module!");
    camera_init(&app->cam);
    if (graphics_init(&app->gfx, app->gpu, app->window) != 0) panic("Failed to initialize graphics!");
    if (gui_init(&app->gui, app->window, app->gpu) != 0) panic("Failed to initialize GUI!");

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(app->gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    add_body(app, copy_pass, &(SimulationAddBodyInfo) {
         .position = (HMM_Vec2) { .X = 0.0f, .Y = -100.0f },
         .velocity = (HMM_Vec2) { .X = -25.0f, .Y = 0.0f },
         .mass = 50.0f,
         .movable = true,
     }, &(SDL_FColor) { 1.0f, 0.0f, 1.0f, 1.0f });

    add_body(app, copy_pass, &(SimulationAddBodyInfo) {
         .position = (HMM_Vec2) { .X = 0.0f, .Y = 100.0f },
         .velocity = (HMM_Vec2) { .X = 25.0f, .Y = 0.0f },
         .mass = 50.0f,
         .movable = true,
     }, &(SDL_FColor) { 0.0f, 1.0f, 1.0f, 1.0f });

    add_body(app, copy_pass, &(SimulationAddBodyInfo) {
         .position = (HMM_Vec2) { .X = 100.0f, .Y = 0.0f },
         .velocity = (HMM_Vec2) { .Y = 10.0f },
         .mass = 50.0f,
         .movable = true,
     }, &(SDL_FColor) { 1.0f, 1.0f, 0.0f, 1.0f });

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Application *app = appstate;
    static u64 last_tick = 0;
    static f32 accumulator = 0.0f;

    if (last_tick == 0) last_tick = SDL_GetTicksNS();
    const u64 current_tick = SDL_GetTicksNS();
    const f32 delta_time = (f32)(current_tick - last_tick) / (f32) SDL_NS_PER_SECOND;
    last_tick = current_tick;

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(app->gpu);
    const SDL_GPUStorageBufferReadWriteBinding bindings[] = {
        { .buffer = app->sim.positions.buffer, .cycle = false },
        { .buffer = app->sim.velocities.buffer, .cycle = false },
        { .buffer = app->trails.array.buffer, .cycle = false },
        { .buffer = app->trajectories.positions.buffer, .cycle = false },
        { .buffer = app->trajectories.velocities.buffer, .cycle = false },
    };

    SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(
        command_buffer,
        NULL, 0,
        bindings, sizeof(bindings) / sizeof(SDL_GPUStorageBufferReadWriteBinding)
    );


    accumulator += delta_time;
    while (accumulator >= app->options.fixed_delta_time) {
        simulation_update(&app->sim, command_buffer, compute_pass, app->options.fixed_delta_time);
        trails_update(&app->trails, command_buffer, compute_pass, &app->sim);
        trajectories_update(&app->trajectories, command_buffer, compute_pass, &app->sim, PREDICTION_DELTA_TIME_MULTIPLIER * app->options.fixed_delta_time); // FIXME: why does changing this to use &info break everything?
        camera_update(&app->cam, app->window, app->gpu, &app->sim);
        accumulator -= app->options.fixed_delta_time;
    }

    SDL_EndGPUComputePass(compute_pass);

    gui_update(&(GuiUpdateInfo) {
        .app = &app->options,
        .sim = &app->sim,
        .trajectories = &app->trajectories,
        .gfx = &app->gfx,
    });

    graphics_draw(&app->gfx, command_buffer, &(GraphicsDrawInfo) {
        .window = app->window,
        .gpu = app->gpu,
        .sim = &app->sim,
        .trails = &app->trails,
        .trajectories = &app->trajectories,
        .cam = &app->cam,
    });

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Application *app = appstate;
    UNUSED(app);

    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    gui_event(event);
    if (!app->gui.io->WantCaptureMouse) camera_mouse(&app->cam, event);
    if (!app->gui.io->WantCaptureKeyboard) camera_keyboard(&app->cam, event, &app->sim);
    return SDL_APP_CONTINUE;
}

static void add_body(Application *app, SDL_GPUCopyPass *copy_pass, const SimulationAddBodyInfo *sim_info, SDL_FColor *color) {
    simulation_add_body(&app->sim, app->gpu, copy_pass, sim_info);
    trails_add_body(&app->trails, app->gpu, copy_pass, sim_info->position);
    trajectories_add_body(&app->trajectories, app->gpu, copy_pass, sim_info->position);
    graphics_add_body(&app->gfx, app->gpu, copy_pass, color);
}

void SDL_AppQuit(void *appstate, const SDL_AppResult result) {
    UNUSED(result);
    const Application *app = appstate;

    SDL_WaitForGPUIdle(app->gpu);
    SDL_ReleaseWindowFromGPUDevice(app->gpu, app->window);

    simulation_free(&app->sim, app->gpu);
    trails_free(&app->trails, app->gpu);
    trajectories_free(&app->trajectories, app->gpu);
    graphics_free(&app->gfx, app->gpu);
    gui_free();

    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->gpu);
    SDL_Quit();
}

