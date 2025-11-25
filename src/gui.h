#pragma once

#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "lib/raygui.h"
#include "lib/gui_style.h"
#include "lib/types.h"

#include "constants.h"
#include "simulation.h"
#include "draw.h"

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_WINDOW_CLOSEBUTTON_SIZE 18
#define CLOSE_TITLE_SIZE_DELTA_HALF (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - RAYGUI_WINDOW_CLOSEBUTTON_SIZE) / 2

typedef struct {
    Rectangle layout[23];
    Vector2 anchor;
    bool minimized;
    bool moving;

    bool reset;
    bool paused;

    Color color;
    f32 mass;
    bool movable;
    bool create;
    bool previous_create;
} GUIState;

void gui_init(GUIState *gui);
void gui_draw(GUIState *gui, SimulationParameters *simulation, DrawParameters *drawer);

#ifdef GUI_IMPLEMENTATION

void update_layout(GUIState *state);
void gui_init(GUIState *gui) {
    GuiLoadStyleDark();
    gui->anchor = (Vector2){ 24, 24 },
    gui->minimized = false,
    gui->moving = false,

    gui->paused = false,

    gui->color = COLOR_DEFAULT,
    gui->mass = MASS_DEFAULT,
    gui->movable = true,
    gui->create = false,
    gui->previous_create = false,

    update_layout(gui);
}

void gui_draw(GUIState *gui, SimulationParameters *simulation, DrawParameters *drawer) {
    gui->previous_create = gui->create;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !gui->moving) {
        Rectangle title_collision_rect = {
            gui->anchor.x,
            gui->anchor.y,
            gui->layout[0].width - (RAYGUI_WINDOW_CLOSEBUTTON_SIZE + (f32) CLOSE_TITLE_SIZE_DELTA_HALF),
            RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT
        };
        if(CheckCollisionPointRec(GetMousePosition(), title_collision_rect)) gui->moving = true;
    }

    if (gui->moving) {
        Vector2 mouse_delta = GetMouseDelta();
        gui->anchor.x += mouse_delta.x;
        gui->anchor.y += mouse_delta.y;

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            gui->moving = false;
            if (gui->anchor.x < 0) gui->anchor.x = 0;
            else if (gui->anchor.x > GetScreenWidth() - gui->layout[0].width) gui->anchor.x = GetScreenWidth() - gui->layout[0].width;
            if (gui->anchor.y < 0) gui->anchor.y = 0;
            else if (gui->anchor.y > GetScreenHeight()) gui->anchor.y = GetScreenHeight() - RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
        }

        update_layout(gui);
    }

    if (gui->minimized) {
        GuiStatusBar((Rectangle){ gui->anchor.x, gui->anchor.y, gui->layout[0].width, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT }, "N-Body Simulation");
        if (GuiButton((Rectangle){ gui->anchor.x + gui->layout[0].width - RAYGUI_WINDOW_CLOSEBUTTON_SIZE - (f32) CLOSE_TITLE_SIZE_DELTA_HALF,
                                   gui->anchor.y + (f32) CLOSE_TITLE_SIZE_DELTA_HALF,
                                   RAYGUI_WINDOW_CLOSEBUTTON_SIZE,
                                   RAYGUI_WINDOW_CLOSEBUTTON_SIZE },
                                   "#120#")) {
            gui->minimized = false;
        }
    } else {
        gui->minimized = GuiWindowBox(gui->layout[0], "N-Body Simulation");

        GuiGroupBox(gui->layout[1], "Controls");
        gui->reset = GuiButton(gui->layout[2], "Reset"); 
        GuiToggle(gui->layout[3], "Pause", &gui->paused);

        GuiGroupBox(gui->layout[4], "Simulation Parameters");
        GuiSlider(gui->layout[5], "Gravity", NULL, &simulation->gravity, GRAVITY_MIN, GRAVITY_MAX);
        GuiSlider(gui->layout[6], "Density", NULL, &simulation->density, DENSITY_MIN, DENSITY_MAX);
        GuiSlider(gui->layout[7], "Softening", NULL, &simulation->softening, SOFTENING_MIN, SOFTENING_MAX);
        GuiToggleGroup(gui->layout[8], "Euler;Verlet;RK4", (int*) &simulation->integrator);
        GuiToggleGroup(gui->layout[9], "None;Merge;Elastic", (int*) &simulation->collisions);
        GuiToggleGroup(gui->layout[10], "Direct;Barnes-Hut", (int*) &simulation->barnes_hut);

        GuiGroupBox(gui->layout[11], "Drawing Controls");
        GuiSliderBar(gui->layout[12], "Trail Length", NULL, &drawer->trail, TRAIL_MIN, TRAIL_MAX);
        GuiCheckBox(gui->layout[13], "Relative Trail", &drawer->draw_relative);
        GuiCheckBox(gui->layout[14], "Draw Field Grid", &drawer->draw_field_grid);
        GuiCheckBox(gui->layout[15], "Draw Velocity", &drawer->draw_velocity);
        GuiCheckBox(gui->layout[16], "Draw Net Force", &drawer->draw_net_force);
        GuiCheckBox(gui->layout[17], "Draw Forces", &drawer->draw_forces);

        GuiGroupBox(gui->layout[18], "New / Edit Body");
        GuiColorPicker(gui->layout[19], NULL, &gui->color);
        GuiSlider(gui->layout[20], "Mass", NULL, &gui->mass, MASS_MIN, MASS_MAX);
        GuiCheckBox(gui->layout[21], "Moveable?", &gui->movable);
        GuiToggle(gui->layout[22], "Create!", &gui->create);
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
