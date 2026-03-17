#ifndef N_BODY_GUI
#define N_BODY_GUI

typedef struct Simulation Simulation;
typedef struct Camera Camera;
typedef struct Ghost Ghost;
typedef struct Trajectories Trajectories;
typedef struct Graphics Graphics;

#include <stdbool.h>
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_events.h"
#include "dcimgui.h"
#include "types.h"

typedef struct {
    f32 fixed_delta_time;
} ApplicationOptions;

typedef struct {
    ImGuiIO *io;
} Gui;

void gui_init(Gui *gui, SDL_Window *window, SDL_GPUDevice *gpu);
typedef struct {
    ApplicationOptions *app;
    Simulation *sim;
    Ghost *ghost;
    Trajectories *trajectories;
    Camera *cam;
    Graphics *gfx;
} GuiUpdateInfo;
void gui_update(const GuiUpdateInfo *info);
void gui_event(const SDL_Event *event);
void gui_free(void);

#endif
