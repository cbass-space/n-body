// based on https://github.com/dearimgui/dear_bindings/tree/main/examples/example_sdl3_sdlgpu3
#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#define SDL_UTILS_IMPLEMENTATION
#include "stb_ds.h"
#include "main.h"

i32 main(void) {
    Application app = { 0 };
    if (app_init(&app) != 0) return -1;

    bool done = false;
    u64 last_tick = SDL_GetTicksNS();
    f64 accumulator = 0;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            app_event(&app, &event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(app.window)) done = true;
        }

        if (SDL_GetWindowFlags(app.window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        const u64 current_tick = SDL_GetTicksNS();
        const f64 delta_time = (f64)(current_tick - last_tick) / (f64) SDL_NS_PER_SECOND;
        last_tick = current_tick;

        accumulator += delta_time;
        while (accumulator >= app.options.fixed_delta_time) {
            if (!app.options.paused) app_fixed_update(&app, app.options.fixed_delta_time);
            accumulator -= app.options.fixed_delta_time;
        }

        app_update(&app);
        app_draw(&app);
    }

    app_free(&app);
    return 0;
}

i32 panic(const char *location, const char *message) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s\n", location, message);
    return -1;
}

i32 app_panic(Application *app, const char *location, const char *message) {
    app_free(app);
    panic(location, message);
    return -1;
}

i32 app_init(Application *app) {
    app->options = (ApplicationOptions) {
        .fixed_delta_time = FIXED_DELTA_TIME_DEFAULT,
        .paused = false,
        .create_mode = false,
        .create_body = (NewBody) {
            .mass = MASS_DEFAULT,
            .movable = true,
            .color = COLOR_DEFAULT
        }
    };

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return app_panic(app, "SDL_Init() in app_init()", SDL_GetError());
    const f32 main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    app->window = SDL_CreateWindow("N-Body Simulation", (i32)(WIDTH_DEFAULT * main_scale), (i32)(HEIGHT_DEFAULT * main_scale), window_flags);
    if (!app->window) return app_panic(app, "SDL_CreateWindow() in app_init()", SDL_GetError());
    SDL_ShowWindow(app->window);

    SDL_SetLogPriority(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_VERBOSE);

    app->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (!app->gpu) return app_panic(app, "SDL_CreateGPU() in app_init()", SDL_GetError());
    if (!SDL_ClaimWindowForGPUDevice(app->gpu, app->window)) return app_panic(app, "SDL_ClaimWindowForGPU() in app_init()", SDL_GetError());
    SDL_SetGPUSwapchainParameters(app->gpu, app->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // initialize modules
    camera_init(&app->cam);
    if (simulation_init(&app->sim) != 0) return app_panic(app, "simulation_init() in app_init()", "Failed to initialize the simulation!");
    if (graphics_init(&app->gfx, app->gpu, app->window) != 0) return app_panic(app, "graphics_init() in app_init()", "Failed to initialize graphics!");
    if (gui_init(&app->gui, app->window, app->gpu) != 0) return app_panic(app, "gui_init() in app_init()", "Failed to initialize GUI!");

    const NewBody body_a = {
        .position = (HMM_Vec2) { .X = 0.0f, .Y = 100.0f },
        .velocity = (HMM_Vec2) { .X = 30.0f, .Y = 0.0f },
        .mass = 100.0f,
        .movable = true,
        .color = (SDL_FColor) { 1.0f, 1.0f, 1.0f, 1.0f },
    };

    const NewBody body_b = {
        .position = (HMM_Vec2) { .X = 0.0f, .Y = -100.0f },
        .velocity = (HMM_Vec2) { .X = -30.0f, .Y = 0.0f },
        .mass = 100.0f,
        .movable = true,
        .color = (SDL_FColor) { 1.0f, 1.0f, 1.0f, 1.0f },
    };

    simulation_push_body(&app->sim, &body_a);
    simulation_push_body(&app->sim, &body_b);
    graphics_push_body(&app->gfx, app->gpu, &body_a);
    graphics_push_body(&app->gfx, app->gpu, &body_b);

    return 0;
}

void app_update(Application *app) {
    gui_update(&app->options, &app->sim, &app->gfx, &app->cam);
    camera_update(&app->cam, &app->sim);
}

void app_fixed_update(Application *app, const f64 delta_time) {
    simulation_update(&app->sim, delta_time);
}

void app_event(Application *app, const SDL_Event *event) {
    gui_event(event);

    if (!app->gui.io->WantCaptureMouse) {
        camera_mouse(&app->cam, event, app->window);
    }

    if (!app->gui.io->WantCaptureKeyboard) {
        camera_keyboard(&app->cam, event, &app->sim);
    }
}

void app_draw(Application *app) {
    app->gfx.command_buffer = SDL_AcquireGPUCommandBuffer(app->gpu);
    SDL_WaitAndAcquireGPUSwapchainTexture(app->gfx.command_buffer, app->window, &app->gfx.swapchain, NULL, NULL);
    if (!app->gfx.swapchain) {
        SDL_SubmitGPUCommandBuffer(app->gfx.command_buffer);
        return;
    };

    graphics_draw(&app->gfx, app->gpu, app->window, &app->sim, &app->cam);
    gui_draw(&app->gui, &app->gfx);

    SDL_SubmitGPUCommandBuffer(app->gfx.command_buffer);
}

void app_free(Application *app) {
    SDL_WaitForGPUIdle(app->gpu);
    SDL_ReleaseWindowFromGPUDevice(app->gpu, app->window);

    simulation_free(&app->sim);
    graphics_free(&app->gfx, app->gpu);
    gui_free();

    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->gpu);
    SDL_Quit();
}
