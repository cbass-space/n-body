#include "stb_ds.h"
#include "dcimgui.h"
#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#include "main.h"

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
    ImGuiStyle* style = ImGui_GetStyle();
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

static void HelpMarker(const char* desc);
void gui_update(const GuiUpdateInfo *info) {
    cImGui_ImplSDLGPU3_NewFrame();
    cImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();

    ImGui_Begin("N-Body Simulator", NULL, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui_CollapsingHeader("Welcome!", 0)) {
        ImGui_Text("Thanks for coming :)");
    }

    if (ImGui_CollapsingHeader("Create Body", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui_Checkbox("Body creation mode!", &info->ghost->mode);
        HelpMarker("To create a new body: activate body creation mode, hold right click where you want to create the new body, drag out its velocity, and release!");
        ImGui_BeginDisabled(!info->ghost->mode);
        ImGui_DragFloat("Mass", &info->ghost->mass);
        HelpMarker("The mass of the new body.");
        ImGui_ColorEdit3("Color", (f32*) &info->ghost->color, 0);
        HelpMarker("The color of the new body.");
        ImGui_Checkbox("Movable", &info->ghost->movable);
        HelpMarker("Whether the body should be simulated or remain in place.");
        ImGui_EndDisabled();
    }

    if (ImGui_CollapsingHeader("Simulation Inspector", 0)) {
        ImGui_SeparatorText("Edit Bodies");
        for (usize i = 0; i < arrlenu(info->sim->r); i++) {
            ImGui_PushIDInt((i32) i);
            if (ImGui_TreeNodeExStr("", ImGuiTreeNodeFlags_DefaultOpen, "Body %d", (i32) i)) {
                if (ImGui_ColorEdit3("Color", (f32*) &info->gfx->colors[i], 0)) info->gfx->dirty_flag = (GraphicsDirtyFlag) {
                    .type = DIRTY_COLOR,
                    .index = i,
                    .color = info->gfx->colors[i]
                };

                if (ImGui_DragFloat("Mass", &info->sim->m[i])) info->gfx->dirty_flag = (GraphicsDirtyFlag) {
                    .type = DIRTY_MASS,
                    .index = i,
                    .mass = info->sim->m[i]
                };

                ImGui_DragFloat2("Position", (f32*) &info->sim->r[i]);
                ImGui_DragFloat2("Velocity", (f32*) &info->sim->v[i]);

                if (ImGui_Checkbox("Movable", &info->sim->movable[i])) info->gfx->dirty_flag = (GraphicsDirtyFlag) {
                    .type = DIRTY_MOVABLE,
                    .index = i,
                    .movable = info->sim->movable[i]
                };

                if (ImGui_Button("Follow Body")) info->cam->target = i;
                ImGui_TreePop();
            }
            ImGui_PopID();
        }
    }

    if (ImGui_CollapsingHeader("Controls and Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui_SeparatorText("Controls");
        ImGui_Checkbox("Pause Simulation", &info->app->paused);
        ImGui_Button("Reset Simulation");
        SimulationOptions *sim = &info->sim->options;

        ImGui_SeparatorText("Simulation Options");
        ImGui_DragFloat("Time Step", &info->app->fixed_delta_time);
        ImGui_DragFloat("Gravity Coefficient", &sim->gravity);
        HelpMarker("Strength of the gravitational force between two bodies.");
        ImGui_DragFloat("Softening Coefficient", &sim->softening);
        HelpMarker("How much to reduce the gravitational force between two bodies on close encounter for numerical stability.");
        ImGui_DragFloat("Density Coefficient", &sim->density);
        HelpMarker("How dense each body is.");
        const char *integrators[] = { "Semi-Implicit Euler", "Velocity Verlet", "Runge-Kutta 4" };
        ImGui_ComboChar("Integrator", (i32*) &sim->integrator, integrators, IM_ARRAYSIZE(integrators));
        HelpMarker("The algorithm used to calculate the new velocity and position of each body given the acceleration. Euler is the most performant, Verlet is more accurate while still conserving energy, and RK4 is the most accurate across short time spans but does not conserve energy.");
        const char *collisions[] = { "No Collisions", "Merge on Collision", "Bounce on Collision" };
        ImGui_ComboChar("Collisions", (i32*) &sim->collisions, collisions, IM_ARRAYSIZE(collisions));
        HelpMarker("How to handle body collisions.");
        if (sim->collisions == COLLISIONS_BOUNCE) {
            static f32 cor = 1.0f;
            ImGui_SliderFloat("Restitution Coefficient", &cor, 0.0f, 1.0f);
            HelpMarker("A coefficient of 0.0 leads to a perfectly inelastic collision, while a coefficient of 1.0 is a perfectly elastic collision.");
        }
        ImGui_Checkbox("Use Barnes-Hut Optimization", &sim->barnes_hut);
        HelpMarker("Whether to use a quadtree to calculate the gravitational force on a body (more performant) or calculate the force directly (more accurate).");
        if (sim->barnes_hut) {
            static bool draw = false;
            static u8 max_leaves = 1;
            ImGui_SeparatorText("Barnes-Hut Parameters");
            ImGui_Checkbox("Draw quadtree", &draw);
            ImGui_SliderInt("Bodies per Leaf Node", (i32*) &max_leaves, 1, 256);
            HelpMarker("The maximum number of bodies per leaf node of the quadtree.");
        }

        ImGui_SeparatorText("Drawing Options");
        ImGui_ColorEdit3("Space Color", (f32*) &info->gfx->options.clear_color, 0);
        ImGui_SliderFloat("Movable Body Outline", &info->gfx->options.movable_outline, 0.0f, 1.0f);
        HelpMarker("The thickness of the outline around movable bodies.");
        ImGui_SliderFloat("Static Body Outline", &info->gfx->options.static_outline, 0.0f, 1.0f);
        HelpMarker("The thickness of the outline around non-movable bodies.");
        ImGui_SliderFloat("Trail brightness", &info->gfx->options.trail_brightness, 0.0f, 1.0f);
        HelpMarker("The brightness of the trail that each body leaves behind as it moves.");
    }

    ImGui_End();
    ImGui_Render();
}

static void HelpMarker(const char* desc) {
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
