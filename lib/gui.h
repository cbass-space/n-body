#pragma once

#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raymath.h"
#include "raylib/raygui.h"
#include "raylib/gui_style.h"

#include "types.h"

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_WINDOW_CLOSEBUTTON_SIZE 18
#define CLOSE_TITLE_SIZE_DELTA_HALF (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - RAYGUI_WINDOW_CLOSEBUTTON_SIZE) / 2

#define SIZE_DEFAULT 10.0
#define SIZE_MIN 0.0
#define SIZE_MAX 20.0

// TODO: grid size slider?
#define GRID_DEFAULT 100.0

#define GRAVITY_DEFAULT 10000.0
#define GRAVITY_MIN 5000.0
#define GRAVITY_MAX 20000.0

#define MASS_DEFAULT 100.0
#define MASS_MIN 0.0
#define MASS_MAX 600.0

#define TRAIL_DEFAULT 64
#define TRAIL_MAX 512
#define TRAIL_MIN 0

#define COLOR_DEFAULT WHITE

typedef enum {
    RK4,
    Verlet,
} Integrator;

typedef struct {
    Vector2 anchor;
    Rectangle layout[20];
    bool minimized;
    bool moving;

    bool reset;
    bool paused;

    f32 gravity;
    Integrator integrator;

    f32 size;
    f32 trail;
    bool draw_relative;
    bool draw_field_grid;
    bool draw_velocity;
    bool draw_forces;
    bool draw_net_force;

    Color color;
    f32 mass;
    bool movable;
    bool create;
    bool previous_create;
} GUIState;

GUIState gui_init();
void gui_draw(GUIState *state);

#ifdef GUI_IMPLEMENTATION

void update_layout(GUIState *state);
GUIState gui_init() {
    GuiLoadStyleDark();
    GUIState state = {
        .anchor = (Vector2){ 24, 24 },
        .minimized = false,
        .moving = false,

        .paused = false,

        .gravity = GRAVITY_DEFAULT,
        .integrator = RK4,

        .size = SIZE_DEFAULT,
        .trail = TRAIL_DEFAULT,
        .draw_relative = false,
        .draw_field_grid = false,
        .draw_velocity = false,
        .draw_forces = false,
        .draw_net_force = false,

        .color = COLOR_DEFAULT,
        .mass = MASS_DEFAULT,
        .movable = true,
        .create = false,
        .previous_create = false,
    };

    update_layout(&state);
    return state;
}

void gui_draw(GUIState *state) {
    state->previous_create = state->create;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !state->moving) {
        Rectangle title_collision_rect = { state->anchor.x, state->anchor.y, state->layout[0].width - (RAYGUI_WINDOW_CLOSEBUTTON_SIZE + CLOSE_TITLE_SIZE_DELTA_HALF), RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT };
        if(CheckCollisionPointRec(GetMousePosition(), title_collision_rect)) state->moving = true;
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

        GuiGroupBox(state->layout[1], "Controls");
        state->reset = GuiButton(state->layout[2], "Reset"); 
        GuiToggle(state->layout[3], "Pause", &state->paused);

        GuiGroupBox(state->layout[4], "Simulation Parameters");
        GuiSlider(state->layout[5], "Gravity Modifier", NULL, &state->gravity, GRAVITY_MIN, GRAVITY_MAX);
        GuiToggleGroup(state->layout[6], "RK4;Verlet;Barnes Hut", &state->integrator);

        GuiGroupBox(state->layout[7], "Drawing Controls");
        GuiSliderBar(state->layout[8], "Size Modifier", NULL, &state->size, SIZE_MIN, SIZE_MAX);
        GuiSliderBar(state->layout[9], "Trail Length", NULL, &state->trail, TRAIL_MIN, TRAIL_MAX);
        GuiCheckBox(state->layout[10], "Draw Relative Trail", &state->draw_relative);
        GuiCheckBox(state->layout[11], "Draw Field Grid", &state->draw_field_grid);
        GuiCheckBox(state->layout[12], "Draw Velocity", &state->draw_velocity);
        GuiCheckBox(state->layout[13], "Draw Net Force", &state->draw_net_force);
        GuiCheckBox(state->layout[14], "Draw Forces", &state->draw_forces);

        GuiGroupBox(state->layout[15], "New / Edit Body");
        GuiColorPicker(state->layout[16], NULL, &state->color);
        GuiSlider(state->layout[17], "Mass", NULL, &state->mass, MASS_MIN, MASS_MAX);
        GuiCheckBox(state->layout[18], "Moveable?", &state->movable);
        GuiToggle(state->layout[19], "Create!", &state->create);
    }
}

void update_layout(GUIState *state) {
    Vector2 controls = { state->anchor.x + 16, state->anchor.y + 40 };
    Vector2 simulation = { state->anchor.x + 16, state->anchor.y + 112 };
    Vector2 drawing = { state->anchor.x + 16, state->anchor.y + 208 };
    Vector2 body = { state->anchor.x + 16, state->anchor.y + 424 };
    state->layout[0] = (Rectangle) { state->anchor.x, state->anchor.y, 320, 656 };

    // Controls
    state->layout[1] = (Rectangle) { controls.x, controls.y, 288, 56 };
    state->layout[2] = (Rectangle) { controls.x + 16, controls.y + 16, 120, 24 };
    state->layout[3] = (Rectangle) { controls.x + 152, controls.y + 16, 120, 24 };

    // Simulation Parameters
    state->layout[4] = (Rectangle) { simulation.x, simulation.y, 288, 80 };
    state->layout[5] = (Rectangle) { simulation.x + 120, simulation.y + 16, 152, 16 };
    state->layout[6] = (Rectangle) { simulation.x + 16, simulation.y + 48, 85, 16 };

    // Drawing Controls
    state->layout[7] = (Rectangle) { drawing.x, drawing.y, 288, 200 };
    state->layout[8] = (Rectangle) { drawing.x + 120, drawing.y + 16, 152, 16 };
    state->layout[9] = (Rectangle) { drawing.x + 120, drawing.y + 48, 152, 16 };
    state->layout[10] = (Rectangle) { drawing.x + 16, drawing.y + 80, 24, 24 };
    state->layout[11] = (Rectangle) { drawing.x + 16, drawing.y + 120, 24, 24 };
    state->layout[12] = (Rectangle) { drawing.x + 144, drawing.y + 120, 24, 24 };
    state->layout[13] = (Rectangle) { drawing.x + 144, drawing.y + 160, 24, 24 };
    state->layout[14] = (Rectangle) { drawing.x + 16, drawing.y + 160, 24, 24 };
    
    // New / Edit Body
    state->layout[15] = (Rectangle) { body.x, body.y, 288, 216 };
    state->layout[16] = (Rectangle) { body.x + 16, body.y + 16, 232, 112 };
    state->layout[17] = (Rectangle) { body.x + 64, body.y + 144, 208, 16 };
    state->layout[18] = (Rectangle) { body.x + 24, body.y + 176, 24, 24 };
    state->layout[19] = (Rectangle) { body.x + 144, body.y + 176, 128, 24 };
}

#endif
