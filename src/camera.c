#include "camera.h"
#include "SDL3/SDL_gpu.h"
#include "sdl_utils.h"
#include "simulation.h"

#include "stb_ds.h"

SDL_AppResult camera_init(Camera *cam, SDL_Window *window, SDL_GPUDevice *gpu) {
    cam->pipeline = CreateGPUComputePipeline(gpu, "shaders/camera.comp.spv");

    cam->buffer = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo) {
        .size = sizeof(HMM_Vec2) + sizeof(f32),
        .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
    });

    cam->matrices = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo) {
        .size = 2 * sizeof(HMM_Mat4),
        .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
    });

    if (!cam->pipeline) panic("Couldn't create camera compute pipeline!");
    if (!cam->buffer) panic("Couldn't create camera buffer!");
    if (!cam->matrices) panic("Couldn't create camera matrices buffer!");

    const struct {
        HMM_Vec2 position;
        f32 zoom;
        f32 padding;
    } camera = { .zoom = 1.0f };

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    WriteToGPUBuffers(gpu, copy_pass, &(WriteGPUBufferBinding) {
        .buffer = cam->buffer,
        .source = (u8*) &camera,
        .size = sizeof(HMM_Vec2) + sizeof(f32)
    }, 1);

    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(
        command_buffer,
        NULL, 0,
        &(SDL_GPUStorageBufferReadWriteBinding) { .buffer = cam->matrices, .cycle = false }, 1
    );

    i32 width, height;
    SDL_GetWindowSize(window, &width, &height);
    f32 window_size[] = { (f32) width, (f32) height };

    SDL_BindGPUComputePipeline(compute_pass, cam->pipeline);
    SDL_PushGPUComputeUniformData(command_buffer, 0, &window_size, sizeof(window_size));

    SDL_GPUBuffer *bindings[] = { cam->matrices, cam->buffer };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, bindings, 2);
    SDL_DispatchGPUCompute(compute_pass, 1, 1, 1);

    SDL_EndGPUComputePass(compute_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

void camera_free(const Camera *cam, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, cam->pipeline);
    SDL_ReleaseGPUBuffer(gpu, cam->buffer);
    SDL_ReleaseGPUBuffer(gpu, cam->matrices);
}

// void camera_init(Camera *cam) {
//     cam->zoom = 1.0f;
//     cam->target = (usize) -1;
// }
//
// HMM_Vec2 screen_to_world(const Camera *cam, SDL_Window *window, HMM_Vec2 position) {
//     i32 width, height;
//     SDL_GetWindowSize(window, &width, &height);
//     const HMM_Vec2 center = (HMM_Vec2) { .X = (f32) width / 2.0f, .Y = (f32) height / 2.0f };
//     position.Y = (f32) height - position.Y;
//
//     HMM_Vec2 camera_to_point = HMM_SubV2(position, center);
//     camera_to_point = HMM_MulV2F(camera_to_point, cam->zoom);
//     return HMM_AddV2(cam->position, camera_to_point);
// }
//
// HMM_Vec2 world_to_screen(const Camera *cam, SDL_Window *window, const HMM_Vec2 position) {
//     i32 width, height;
//     SDL_GetWindowSize(window, &width, &height);
//     const HMM_Vec2 center = (HMM_Vec2) { .X = (f32) width / 2.0f, .Y = (f32) height / 2.0f };
//
//     HMM_Vec2 camera_to_point = HMM_SubV2(position, cam->position);
//     camera_to_point = HMM_DivV2F(camera_to_point, cam->zoom);
//     HMM_Vec2 screen = HMM_AddV2(center, camera_to_point);
//     screen.Y = (f32) height - screen.Y;
//     return screen;
// }
//
// HMM_Vec2 mouse_world_position(const Camera *cam, SDL_Window *window) {
//     HMM_Vec2 mouse = { 0 };
//     SDL_GetMouseState(&mouse.X, &mouse.Y);
//     return screen_to_world(cam, window, mouse);
// }
//
// void camera_mouse(Camera *cam, const SDL_Event *event, SDL_Window *window, const bool ghost_enabled) {
//     HMM_Vec2 mouse_delta = { 0 };
//     if (SDL_GetRelativeMouseState(&mouse_delta.X, &mouse_delta.Y) & SDL_BUTTON_RMASK) {
//         cam->target = (usize) -1;
//         mouse_delta.Y *= -1.0f; // screen to world coordinate system
//         mouse_delta = HMM_MulV2F(mouse_delta, -cam->zoom);
//         cam->position = HMM_AddV2(cam->position, mouse_delta);
//     }
//
//     if (event->type == SDL_EVENT_MOUSE_WHEEL && !ghost_enabled) {
//         const HMM_Vec2 mouse_old = mouse_world_position(cam, window);
//         cam->zoom *= SDL_expf(event->wheel.y * -0.1f);
//
//         if (cam->target == (usize) -1) {
//             const HMM_Vec2 mouse_new = mouse_world_position(cam, window);
//             const HMM_Vec2 delta = HMM_SubV2(mouse_new, mouse_old);
//             cam->position = HMM_SubV2(cam->position, delta);
//         }
//     }
// }
//
// void camera_keyboard(Camera *cam, const SDL_Event *event, const Simulation *sim) {
//     if (event->type != SDL_EVENT_KEY_DOWN) return;
//     if (event->key.scancode == SDL_SCANCODE_RIGHTBRACKET) cam->target = (cam->target + 1) % arrlenu(sim->positions);
//     if (event->key.scancode == SDL_SCANCODE_LEFTBRACKET) cam->target = (cam->target - 1 + arrlenu(sim->positions)) % arrlenu(sim->positions);
// }
//
// void camera_update(Camera *cam, const Simulation *sim) {
//     if (cam->target == (usize) -1) return;
//     cam->position = sim->positions[cam->target];
// }
