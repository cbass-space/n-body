#pragma once

#include <string.h>

#include "raylib.h"
#include "lib/raygui.h"
#include "lib/gui_style.h"

#include "lib/types.h"

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_WINDOW_CLOSEBUTTON_SIZE 18
#define CLOSE_TITLE_SIZE_DELTA_HALF (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - RAYGUI_WINDOW_CLOSEBUTTON_SIZE) / 2

#define GRAVITY_DEFAULT 10000.0
#define GRAVITY_MIN 5000.0
#define GRAVITY_MAX 20000.0

#define SOFTENING_DEFAULT 0.01
#define SOFTENING_MIN 0.00
#define SOFTENING_MAX 0.05

#define INTEGRATOR_DEFAULT VERLET

#define DENSITY_DEFAULT 0.001
#define DENSITY_MAX 0.005
#define DENSITY_MIN 0.000125

#define TRAIL_DEFAULT 128
#define TRAIL_MAX 512
#define TRAIL_MIN 0

// TODO: grid size slider?
#define GRID_DEFAULT 100.0

#define COLOR_DEFAULT WHITE

#define MASS_DEFAULT 100.0
#define MASS_MIN 0.0
#define MASS_MAX 600.0

typedef enum {
    EULER,
    VERLET,
    RK4,
} Integrator;

typedef enum {
    NONE,
    MERGE,
    ELASTIC
} Collisions;

typedef struct {
    Vector2 anchor;
    Rectangle layout[23];
    bool minimized;
    bool moving;

    bool reset;
    bool paused;

    f32 gravity;
    f32 softening;
    f32 density;
    Integrator integrator;
    bool barnes_hut;
    Collisions collisions;

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
        .softening = SOFTENING_DEFAULT,
        .density = DENSITY_DEFAULT,
        .integrator = INTEGRATOR_DEFAULT,
        .barnes_hut = false,
        .collisions = NONE,

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
        Rectangle title_collision_rect = {
            state->anchor.x,
            state->anchor.y,
            state->layout[0].width - (RAYGUI_WINDOW_CLOSEBUTTON_SIZE + CLOSE_TITLE_SIZE_DELTA_HALF),
            RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT
        };
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
        if (GuiButton((Rectangle){ state->anchor.x + state->layout[0].width - RAYGUI_WINDOW_CLOSEBUTTON_SIZE - (f32)CLOSE_TITLE_SIZE_DELTA_HALF,
                                   state->anchor.y + (f32)CLOSE_TITLE_SIZE_DELTA_HALF,
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
        GuiSlider(state->layout[5], "Gravity", NULL, &state->gravity, GRAVITY_MIN, GRAVITY_MAX);
        GuiSlider(state->layout[6], "Softening", NULL, &state->softening, SOFTENING_MIN, SOFTENING_MAX);
        GuiSlider(state->layout[7], "Density", NULL, &state->density, DENSITY_MIN, DENSITY_MAX);
        GuiToggleGroup(state->layout[8], "Euler;Verlet;RK4", (int *)&state->integrator);
        GuiToggleGroup(state->layout[9], "None;Merge;Elastic", (int *)&state->collisions);
        GuiToggleGroup(state->layout[10], "Direct;Barnes-Hut", (bool *)&state->barnes_hut);

        GuiGroupBox(state->layout[11], "Drawing Controls");
        GuiSliderBar(state->layout[12], "Trail Length", NULL, &state->trail, TRAIL_MIN, TRAIL_MAX);
        GuiCheckBox(state->layout[13], "Relative Trail", &state->draw_relative);
        GuiCheckBox(state->layout[14], "Draw Field Grid", &state->draw_field_grid);
        GuiCheckBox(state->layout[15], "Draw Velocity", &state->draw_velocity);
        GuiCheckBox(state->layout[16], "Draw Net Force", &state->draw_net_force);
        GuiCheckBox(state->layout[17], "Draw Forces", &state->draw_forces);

        GuiGroupBox(state->layout[18], "New / Edit Body");
        GuiColorPicker(state->layout[19], NULL, &state->color);
        GuiSlider(state->layout[20], "Mass", NULL, &state->mass, MASS_MIN, MASS_MAX);
        GuiCheckBox(state->layout[21], "Moveable?", &state->movable);
        GuiToggle(state->layout[22], "Create!", &state->create);
    }
}

// TODO: this is so bad
void update_layout(GUIState *state) {
    Vector2 controls = { state->anchor.x + 16, state->anchor.y + 40 };
    Vector2 simulation = { state->anchor.x + 16, state->anchor.y + 112 };
    Vector2 drawing = { state->anchor.x + 16, state->anchor.y + 304 };
    Vector2 body = { state->anchor.x + 16, state->anchor.y + 472 };

    state->layout[0] = (Rectangle) { state->anchor.x, state->anchor.y, 320, 704 };

    // Controls
    state->layout[1] = (Rectangle) { controls.x, controls.y, 288, 56 };
    state->layout[2] = (Rectangle) { controls.x + 16, controls.y + 16, 120, 24 };
    state->layout[3] = (Rectangle) { controls.x + 152, controls.y + 16, 120, 24 };

    // Simulation Parameters
    state->layout[4] = (Rectangle) { simulation.x, simulation.y, 288, 176 };
    state->layout[5] = (Rectangle) { simulation.x + 120, simulation.y + 16, 152, 16 };
    state->layout[6] = (Rectangle) { simulation.x + 120, simulation.y + 40, 152, 16 };
    state->layout[7] = (Rectangle) { simulation.x + 120, simulation.y + 64, 152, 16 };
    state->layout[8] = (Rectangle) { simulation.x + 16, simulation.y + 96, 84, 16 };
    state->layout[9] = (Rectangle) { simulation.x + 16, simulation.y + 120, 84, 16 };
    state->layout[10] = (Rectangle) { simulation.x + 16, simulation.y + 144, 127, 16 };

    // Drawing Controls
    state->layout[11] = (Rectangle) { drawing.x, drawing.y, 288, 152 };
    state->layout[12] = (Rectangle) { drawing.x + 120, drawing.y + 16, 152, 16 };
    state->layout[13] = (Rectangle) { drawing.x + 16, drawing.y + 48, 24, 24 };
    state->layout[14] = (Rectangle) { drawing.x + 16, drawing.y + 80, 24, 24 };
    state->layout[15] = (Rectangle) { drawing.x + 144, drawing.y + 80, 24, 24 };
    state->layout[16] = (Rectangle) { drawing.x + 144, drawing.y + 112, 24, 24 };
    state->layout[17] = (Rectangle) { drawing.x + 16, drawing.y + 112, 24, 24 };
    
    // New / Edit Body
    state->layout[18] = (Rectangle) { body.x, body.y, 288, 216 };
    state->layout[19] = (Rectangle) { body.x + 16, body.y + 16, 232, 112 };
    state->layout[20] = (Rectangle) { body.x + 64, body.y + 144, 208, 16 };
    state->layout[21] = (Rectangle) { body.x + 24, body.y + 176, 24, 24 };
    state->layout[22] = (Rectangle) { body.x + 152, body.y + 176, 120, 24 };
}

#endif
