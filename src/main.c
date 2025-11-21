#include <float.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "constants.h"
#include "simulation.h"
#include "gui.h"
#include "application.h"

#include "draw.h"
#include "input.h"
#include "camera.h"

#define STB_DS_IMPLEMENTATION
#include "raylib.h"
#include "lib/types.h"
#include "lib/stb_ds.h"

i32 main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH_DEFAULT, HEIGHT_DEFAULT, "cbass is confused");
    SetTargetFPS(60);

    Application application = { 0 };
    application_init(&application);

    f32 time_accumulator = 0.0; // https://gafferongames.com/post/fix_your_timestep/
    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        // get input
        if (IsKeyPressed(KEY_R) || application.gui.reset) application_init(&application);
        if (IsKeyPressed(KEY_SPACE)) application.gui.paused = !application.gui.paused;
        if (IsKeyPressed(KEY_ESCAPE) && application.gui.create) application.gui.create = false;
        if (IsKeyPressed(KEY_C)) application.gui.create = !application.gui.create;
        if (IsKeyPressed(KEY_M)) application.gui.movable = !application.gui.movable;
        if (IsKeyPressed(KEY_LEFT_BRACKET)) target_set(&application, (application.planet_target + arrlen(application.simulation.planets) - 1) % arrlen(application.simulation.planets));
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) target_set(&application, (application.planet_target + 1) % arrlen(application.simulation.planets));

        target_click(&application);
        planet_update_gui(&application);

        // update and simulate
        f32 time_step = GetFrameTime();
        time_accumulator += time_step;

        while (time_accumulator >= TIME_STEP) {
            camera_update(&application, TIME_STEP);
            if (!application.gui.paused) simulation_update(&application.simulation, TIME_STEP);
            time_accumulator -= TIME_STEP;
        }

        // draw
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(application.camera);

        for (usize i = 0; i < arrlen(application.simulation.planets); i++) {
            planet_draw(&application, i);
        }

        planet_create(&application);
        planet_creation_draw(&application);
        field_grid_draw(&application);

        EndMode2D();
        gui_draw(&application.gui);
        EndDrawing();
    }

    CloseWindow();

    arrfree(application.simulation.planets);

    return 0;
}

