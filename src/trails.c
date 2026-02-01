#include "trails.h"
#include "constants.h"
#include "simulation.h"

#define TRAIL_SIZE sizeof(HMM_Vec2) * TRAIL_LENGTH

i32 trails_init(Trails *trails, SDL_GPUDevice *gpu) {
    trails->array = CreateGPUArray(gpu, TRAIL_SIZE, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    if (!trails->array.buffer) panic("Could not create trails array!");
    trails->pipeline = CreateGPUComputePipeline(gpu, "shaders/trail.comp.spv");
    if (!trails->pipeline) panic("Could not create trails pipeline!");
    return SDL_APP_CONTINUE;
}

u32 trails_add_body(Trails *trails, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const HMM_Vec2 position) {
    HMM_Vec2 trail[TRAIL_LENGTH];
    for (usize i = 0; i < TRAIL_LENGTH; i++) trail[i] = position;

    AppendGPUArrays(gpu, copy_pass, &(AppendGPUArrayBinding) {
        .array = &trails->array,
        .source = (u8 *) &trail,
        .size = TRAIL_SIZE
    }, 1);

    return trails->body_count++;
}

void trails_update(Trails *trails, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass, const Simulation *sim) {
    if (sim->options.paused) return;
    trails->frame = (trails->frame + 1) % TRAIL_LENGTH;
    SDL_PushGPUComputeUniformData(command_buffer, 0, &trails->frame, sizeof(u32));

    SDL_BindGPUComputePipeline(compute_pass, trails->pipeline);
    SDL_GPUBuffer *buffers[] = { trails->array.buffer, sim->positions.buffer };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, buffers, 2);
    SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
}

void trails_free(const Trails *trails, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUBuffer(gpu, trails->array.buffer);
    SDL_ReleaseGPUComputePipeline(gpu, trails->pipeline);
}
