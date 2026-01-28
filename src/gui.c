#include "gui.h"
#include "simulation.h"
#include "camera.h"
// #include "ghost.h"
#include "trajectory.h"
#include "graphics.h"

#include "stb_ds.h"
#include "dcimgui.h"
#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_sdlgpu3.h"
#include "types.h"

i32 gui_init(Gui *gui, SDL_Window *window, SDL_GPUDevice *gpu) {
    CIMGUI_CHECKVERSION();
    ImGui_CreateContext(NULL);
    gui->io = ImGui_GetIO();
    gui->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    gui->io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    gui->io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    gui->io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_StyleColorsDark(NULL);
    const f32 main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    ImGuiStyle *style = ImGui_GetStyle();
    ImGuiStyle_ScaleAllSizes(style, main_scale);
    style->FontScaleDpi = main_scale;

    cImGui_ImplSDL3_InitForSDLGPU(window);
    cImGui_ImplSDLGPU3_Init(&(ImGui_ImplSDLGPU3_InitInfo) {
        .Device = gpu,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu, window),
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
        .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        .PresentMode = SDL_GPU_PRESENTMODE_VSYNC
    });

    return 0;
}

static void HelpMarker(const char *desc);
// static void gui_create_body(Ghost *ghost);
// static void gui_inspector(const Simulation *sim, Graphics *gfx, Camera *cam);
static void gui_controls(ApplicationOptions *app, SimulationOptions *sim, Trajectories *trajectories, GraphicsOptions *gfx);
void gui_update(const GuiUpdateInfo *info) {
    cImGui_ImplSDLGPU3_NewFrame();
    cImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();
    ImGui_Begin("N-Body Simulator", NULL, ImGuiWindowFlags_AlwaysAutoResize);

    // gui_create_body(info->ghost);
    // gui_inspector(info->sim, info->gfx, info->cam);
    gui_controls(info->app, &info->sim->options, info->trajectories, &info->gfx->options);

    ImGui_End();
    ImGui_Render();
}

// static void gui_create_body(Ghost *ghost) {
//     if (ImGui_CollapsingHeader("Create Body", ImGuiTreeNodeFlags_DefaultOpen)) {
//         ImGui_Checkbox("Body creation mode!", &ghost->enabled);
//         HelpMarker("To create a new body: activate body creation mode, hold right click where you want to create the new body, drag out its velocity, and release!");
//         ImGui_BeginDisabled(!ghost->enabled);
//         ImGui_DragFloat("Mass", &ghost->mass);
//         HelpMarker("The mass of the new body.");
//         ImGui_ColorEdit3("Color", (f32 *) &ghost->color, 0);
//         HelpMarker("The color of the new body.");
//         ImGui_Checkbox("Movable", &ghost->movable);
//         HelpMarker("Whether the body should be simulated or remain in place.");
//         ImGui_EndDisabled();
//     }
// }

// static void gui_inspector(const Simulation *sim, Graphics *gfx, Camera *cam) {
//     if (ImGui_CollapsingHeader("Simulation Inspector", 0)) {
//         ImGui_SeparatorText("Edit Bodies");
//         for (usize i = 0; i < sim->body_count; i++) {
//             ImGui_PushIDInt((i32) i);
//             if (ImGui_TreeNodeExStr("", 0, "Body %d", (i32) i)) {
//                 if (ImGui_ColorEdit3("Color", (f32 *) &gfx->colors[i], 0)) gfx->dirty_flag = (GraphicsDirtyFlag) {
//                     .type = DIRTY_COLOR,
//                     .index = i,
//                     .color = gfx->colors[i]
//                 };
//
//                 if (ImGui_DragFloat("Mass", &sim->masses[i])) gfx->dirty_flag = (GraphicsDirtyFlag) {
//                     .type = DIRTY_MASS,
//                     .index = i,
//                     .mass = sim->masses[i]
//                 };
//
//                 ImGui_DragFloat2("Position", (f32 *) &sim->positions[i]);
//                 ImGui_DragFloat2("Velocity", (f32 *) &sim->velocities[i]);
//
//                 if (ImGui_Checkbox("Movable", &sim->movable[i])) gfx->dirty_flag = (GraphicsDirtyFlag) {
//                     .type = DIRTY_MOVABLE,
//                     .index = i,
//                     .movable = sim->movable[i]
//                 };
//
//                 if (ImGui_Button("Follow Body")) cam->target = i;
//                 ImGui_TreePop();
//             }
//             ImGui_PopID();
//         }
//     }
//
// }
static void gui_controls(ApplicationOptions *app, SimulationOptions *sim, Trajectories *trajectories, GraphicsOptions *gfx) {
    if (ImGui_CollapsingHeader("Controls and Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui_SeparatorText("Controls");
        ImGui_Checkbox("Pause Simulation", &sim->paused);
        ImGui_Button("Reset Simulation");

        ImGui_SeparatorText("Simulation Options");
        ImGui_DragFloat("Time Step", &app->fixed_delta_time);
        ImGui_DragFloat("Gravity Coefficient", &sim->gravity);
        HelpMarker("Strength of the gravitational force between two bodies.");
        ImGui_DragFloat("Softening Coefficient", &sim->softening);
        HelpMarker("How much to reduce the gravitational force between two bodies on close encounter for numerical stability.");
        ImGui_DragFloat("Density Coefficient", &sim->density);
        HelpMarker("How dense each body is.");
        const char *integrators[] = { "Semi-Implicit Euler", "Velocity Verlet", "Runge-Kutta 4" };
        ImGui_ComboChar("Integrator", (i32 *) &sim->integrator, integrators, IM_COUNTOF(integrators));
        HelpMarker("The algorithm used to calculate the new velocity and position of each body given the acceleration. Euler is the most performant, Verlet is more accurate while still conserving energy, and RK4 is the most accurate across short time spans but does not conserve energy.");
        // ImGui_Checkbox("Collisions", &sim->collide);
        // HelpMarker("Whether to handle body collisions (expensive compute!)");

        ImGui_Checkbox("Predict Body Motion", &trajectories->enabled);
        HelpMarker("Simulate planets into the future and draw their trajectories (expensive compute!)");

        ImGui_SeparatorText("Drawing Options");
        ImGui_ColorEdit3("Space Color", (f32 *) &gfx->clear_color, 0);
        ImGui_SliderFloat("Movable Body Outline", &gfx->movable_outline, 0.0f, 1.0f);
        HelpMarker("The thickness of the outline around movable bodies.");
        ImGui_SliderFloat("Static Body Outline", &gfx->static_outline, 0.0f, 1.0f);
        HelpMarker("The thickness of the outline around non-movable bodies.");
        ImGui_SliderFloat("Trail brightness", &gfx->trail_brightness, 0.0f, 1.0f);
        HelpMarker("The brightness of the trail that each body leaves behind as it moves.");
    }
}

static void HelpMarker(const char *desc) {
    ImGui_SameLine();
    ImGui_TextDisabled("(?)");
    if (ImGui_BeginItemTooltip()) {
        ImGui_PushTextWrapPos(ImGui_GetFontSize() * 35.0f);
        ImGui_TextUnformatted(desc);
        ImGui_PopTextWrapPos();
        ImGui_EndTooltip();
    }
}

void gui_event(const SDL_Event *event) {
    cImGui_ImplSDL3_ProcessEvent(event);
}

void gui_free(void) {
    cImGui_ImplSDL3_Shutdown();
    cImGui_ImplSDLGPU3_Shutdown();
    ImGui_DestroyContext(NULL);
}
