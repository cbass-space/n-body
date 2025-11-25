#include <float.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "application.h"

#define STB_DS_IMPLEMENTATION
#include "lib/stb_ds.h"
#include "raylib.h"

i32 main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH_DEFAULT, HEIGHT_DEFAULT, "cbass is confused");
    SetTargetFPS(60);

    Application application = { 0 };
    application_init(&application);

    f32 time_accumulator = 0.0; // https://gafferongames.com/post/fix_your_timestep/
    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        application_input(&application);

        f32 time_step = GetFrameTime();
        time_accumulator += time_step;
        while (time_accumulator >= TIME_STEP) {
            application_update(&application, TIME_STEP);
            time_accumulator -= TIME_STEP;
        }

        application_draw(&application);
    }

    CloseWindow();

    application_free(&application);

    return 0;
}

