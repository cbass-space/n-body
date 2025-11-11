#pragma once

#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raymath.h"
#include "lib/raygui.h"
#include "lib/gui_style.h"

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_WINDOW_CLOSEBUTTON_SIZE 18
#define CLOSE_TITLE_SIZE_DELTA_HALF (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - RAYGUI_WINDOW_CLOSEBUTTON_SIZE) / 2

#define SIZE_DEFAULT 10.0
#define SIZE_MIN 2.0
#define SIZE_MAX 20.0
#define GRAVITY_DEFAULT 10000.0
#define GRAVITY_MIN 5000.0
#define GRAVITY_MAX 20000.0
#define MASS_DEFAULT 100.0
#define MASS_MIN 0.0
#define MASS_MAX 300.0
#define COLOR_DEFAULT WHITE

typedef struct {
    Vector2 anchor;
    Rectangle layout[12];

    bool minimized;
    bool moving;

    float size;
    float gravity;

    Color color;
    float mass;
    bool movable;
    bool create;

    bool reset;
    bool paused;
} GUIState;

static void update_layout(GUIState *state);

GUIState gui_init() {
    GuiLoadStyleDark();
    GUIState state = { 0 };

    state.anchor = (Vector2){ 24, 24 };
    state.minimized = false;
    state.moving = false;

    state.size = SIZE_DEFAULT;
    state.gravity = GRAVITY_DEFAULT;
    state.color = COLOR_DEFAULT;
    state.mass = MASS_DEFAULT;
    state.movable = true;
    state.paused = false;

    update_layout(&state);
    return state;
}

void gui_draw(GUIState *state) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !state->moving) {
        Rectangle title_collision_rect = { state->anchor.x, state->anchor.y, state->layout[0].width - (RAYGUI_WINDOW_CLOSEBUTTON_SIZE + CLOSE_TITLE_SIZE_DELTA_HALF), RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT };
        if(CheckCollisionPointRec(GetMousePosition(), title_collision_rect)) {
            state->moving = true;
        }
    }

    if (state->moving) {
        Vector2 mouse_delta = GetMouseDelta();
        state->anchor.x += mouse_delta.x;
        state->anchor.y += mouse_delta.y;

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state->moving = false;
            if (state->anchor.x < 0) state->anchor.x = 0;
            else if (state->anchor.x > GetScreenWidth() - state->layout[0].width) state->anchor.x = GetScreenWidth() - state->layout[0].width;
            if (state->anchor.y < 0) state->anchor.y = 0;
            else if (state->anchor.y > GetScreenHeight()) state->anchor.y = GetScreenHeight() - RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
        }

        update_layout(state);
    }

    if (state->minimized) {
        GuiStatusBar((Rectangle){ state->anchor.x, state->anchor.y, state->layout[0].width, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT }, "N-Body Simulation");
        if (GuiButton((Rectangle){ state->anchor.x + state->layout[0].width - RAYGUI_WINDOW_CLOSEBUTTON_SIZE - CLOSE_TITLE_SIZE_DELTA_HALF,
                                   state->anchor.y + CLOSE_TITLE_SIZE_DELTA_HALF,
                                   RAYGUI_WINDOW_CLOSEBUTTON_SIZE,
                                   RAYGUI_WINDOW_CLOSEBUTTON_SIZE },
                                   "#120#")) {
            state->minimized = false;
        }
    } else {
        state->minimized = GuiWindowBox(state->layout[0], "N-Body Simulation");
        GuiGroupBox(state->layout[1], "Simulation Parameters");
        GuiSlider(state->layout[2], "Size Modifier", NULL, &state->size, SIZE_MIN, SIZE_MAX);
        GuiSlider(state->layout[3], "Gravity Modifier", NULL, &state->gravity, GRAVITY_MIN, GRAVITY_MAX);

        GuiGroupBox(state->layout[4], "New Body");
        GuiColorPicker(state->layout[5], NULL, &state->color);
        GuiSlider(state->layout[6], "Mass", NULL, &state->mass, MASS_MIN, MASS_MAX);
        GuiCheckBox(state->layout[7], "Moveable?", &state->movable);
        GuiToggle(state->layout[8], "Create!", &state->create);

        GuiGroupBox(state->layout[9], "Controls");
        state->reset = GuiButton(state->layout[10], "Reset"); 
        GuiToggle(state->layout[11], "Pause", &state->paused);
    }
}

static void update_layout(GUIState *state) {
    state->layout[0] = (Rectangle){ state->anchor.x + 0, state->anchor.y + 0, 320, 432 };
    state->layout[1] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 40, 288, 72 };
    state->layout[2] = (Rectangle){ state->anchor.x + 136, state->anchor.y + 56, 152, 16 };
    state->layout[3] = (Rectangle){ state->anchor.x + 136, state->anchor.y + 80, 152, 16 };
    state->layout[4] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 128, 288, 216 };
    state->layout[5] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 144, 232, 112 };
    state->layout[6] = (Rectangle){ state->anchor.x + 64, state->anchor.y + 272, 120, 16 };
    state->layout[7] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 304, 24, 24 };
    state->layout[8] = (Rectangle){ state->anchor.x + 168, state->anchor.y + 304, 120, 24 };
    state->layout[9] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 360, 288, 56 };
    state->layout[10] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 376, 120, 24 };
    state->layout[11] = (Rectangle){ state->anchor.x + 168, state->anchor.y + 376, 120, 24 };
}
