#include "constants.h"
#include "simulation.h"
// #include "camera.h"
// #include "ghost.h"
// #include "prediction.h"
#include "graphics.h"
// #include "gui.h"

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_gpu.h"
#include "types.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define UNUSED(x) (void)(x)

typedef struct {
    f32 fixed_delta_time;
} ApplicationOptions;

typedef struct {
    ApplicationOptions options;
    SDL_Window *window;
    SDL_GPUDevice *gpu;
    Simulation sim;
    // Camera cam;
    // Ghost ghost;
    // Predictions predictions;
    Graphics gfx;
    // Gui gui;
} Application;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    UNUSED(argc); UNUSED(argv);

    Application *app = SDL_calloc(1, sizeof(*app));
    *appstate = app;
    app->options = (ApplicationOptions) { .fixed_delta_time = FIXED_DELTA_TIME_DEFAULT };

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return panic("SDL_Init() in app_init()", SDL_GetError());
    const f32 main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    app->window = SDL_CreateWindow("N-Body Simulation", (i32)(WIDTH_DEFAULT * main_scale), (i32)(HEIGHT_DEFAULT * main_scale), window_flags);
    if (!app->window) return panic("SDL_CreateWindow() in app_init()", SDL_GetError());
    SDL_ShowWindow(app->window);

    SDL_SetLogPriority(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_VERBOSE);

    app->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (!app->gpu) return panic("SDL_CreateGPU() in app_init()", SDL_GetError());
    if (!SDL_ClaimWindowForGPUDevice(app->gpu, app->window)) return panic("SDL_ClaimWindowForGPU() in app_init()", SDL_GetError());
    SDL_SetGPUSwapchainParameters(app->gpu, app->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // initialize modules
    simulation_init(&app->sim, app->gpu);
    simulation_add_body(&app->sim, app->gpu, &(SimulationAddBodyInfo) {
        .position = (HMM_Vec2) { 0 },
        .velocity = (HMM_Vec2) { .X = 1.0f, .Y = 0.0f },
        .mass = 100.0f,
        .movable = true,
    });

    // camera_init(&app->cam);
    // ghost_init(&app->ghost);
    // prediction_init(&app->predictions);
    if (graphics_init(&app->gfx, app->gpu, app->window) != 0) return panic("graphics_init() in app_init()", "Failed to initialize graphics!");
    // if (gui_init(&app->gui, app->window, app->gpu) != 0) return panic("gui_init() in app_init()", "Failed to initialize GUI!");

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

    accumulator += delta_time;
    while (accumulator >= app->options.fixed_delta_time) {
        simulation_update(&app->sim, app->gpu, app->options.fixed_delta_time);
        // prediction_update(&app->predictions, &app->sim, &app->ghost, PREDICTION_DELTA_TIME_MULTIPLIER * app->options.fixed_delta_time);
        // camera_update(&app->cam, &app->sim);
        accumulator -= app->options.fixed_delta_time;
    }

    // ghost_update(&app->ghost, app->window, &app->sim, &app->cam);

    // gui_update(&(GuiUpdateInfo) {
    //     .app = &app->options,
    //     .sim = &app->sim,
    //     .cam = &app->cam,
    //     .ghost = &app->ghost,
    //     .predictions = &app->predictions,
    //     .gfx = &app->gfx,
    // });

    graphics_draw(&app->gfx, &(GraphicsDrawInfo) {
        .gpu = app->gpu,
        .window = app->window,
        .sim = &app->sim,
        // .cam = &app->cam,
        // .ghost = &app->ghost,
        // .predictions = &app->predictions
    });

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Application *app = appstate;
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    // gui_event(event);

    // if (!app->gui.io->WantCaptureMouse) {
    //     camera_mouse(&app->cam, event, app->window, app->ghost.enabled);
    //     if (ghost_mouse(&app->ghost, event)) {
    //         const usize index = simulation_add_body(&app->sim, &(SimulationAddBodyInfo) {
    //             .position = app->ghost.position,
    //             .velocity = app->ghost.velocity,
    //             .mass = app->ghost.mass,
    //             .movable = app->ghost.movable
    //         });
    //
    //         graphics_add_body(&app->gfx, &(GraphicsAddBodyInfo) {
    //             .gpu = app->gpu,
    //             .color = app->ghost.color,
    //             .sim = &app->sim,
    //             .index = index
    //         });
    //     }
    // }

    // if (!app->gui.io->WantCaptureKeyboard) {
    //     camera_keyboard(&app->cam, event, &app->sim);
    //     ghost_keyboard(&app->ghost, event);
    //     if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_SPACE) app->sim.options.paused = !app->sim.options.paused;
    // }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    UNUSED(result);

    Application *app = appstate;

    SDL_WaitForGPUIdle(app->gpu);
    SDL_ReleaseWindowFromGPUDevice(app->gpu, app->window);

    simulation_free(&app->sim, app->gpu);
    // prediction_free(&app->predictions);
    graphics_free(&app->gfx, app->gpu);
    // gui_free();

    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->gpu);
    SDL_Quit();
}
