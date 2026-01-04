#ifndef SDL_UTILS
#define SDL_UTILS

#include "types.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3_shadercross/SDL_shadercross.h"

SDL_GPUColorTargetBlendState BLEND_STATE;

typedef struct {
    SDL_Window *window;
    const char *vertex_shader_path;
    const char *fragment_shader_path;
    SDL_GPUPrimitiveType primitive_type;
} CreateGPUGraphicsPipelineInfo;
SDL_GPUGraphicsPipeline *CreateGPUGraphicsPipeline(SDL_GPUDevice *gpu, const CreateGPUGraphicsPipelineInfo *info);
SDL_GPUShader *LoadSPIRVShader(SDL_GPUDevice *gpu, const char *shader_path);

typedef struct {
    SDL_GPUBuffer *buffer;
    const u8 *source;
    u32 size;
    u32 source_offset;
    u32 buffer_offset;
} UploadGPUBufferBinding;
void UploadIntoGPUBuffers(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const UploadGPUBufferBinding *bindings, usize num_bindings);

typedef struct {
    SDL_GPUBuffer *buffer;
    SDL_GPUBufferCreateInfo info;
    u32 used;
} GPUArray;
GPUArray CreateGPUArray(SDL_GPUDevice *gpu, u32 size);
void ExpandGPUArray(GPUArray *array, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, u32 size);

typedef struct {
    GPUArray *array;
    const u8 *source;
    u32 size;
    u32 source_offset;
} AppendGPUArrayBinding;
void AppendGPUArrays(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const AppendGPUArrayBinding *bindings, usize num_bindings);

#endif

// #define SDL_UTILS_IMPLEMENTATION
#ifdef SDL_UTILS_IMPLEMENTATION

SDL_GPUColorTargetBlendState BLEND_STATE = {
    .enable_blend = true,
    .color_blend_op = SDL_GPU_BLENDOP_ADD,
    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};

SDL_GPUGraphicsPipeline *CreateGPUGraphicsPipeline(SDL_GPUDevice *gpu, const CreateGPUGraphicsPipelineInfo *info) {
    SDL_GPUShader *vertex_shader = LoadSPIRVShader(gpu, info->vertex_shader_path);
    SDL_GPUShader *fragment_shader = LoadSPIRVShader(gpu, info->fragment_shader_path);
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &(SDL_GPUGraphicsPipelineCreateInfo) {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .primitive_type = info->primitive_type,
        .target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
            .num_color_targets = 1,
            .color_target_descriptions = &(SDL_GPUColorTargetDescription) {
                .format = SDL_GetGPUSwapchainTextureFormat(gpu, info->window),
                .blend_state = BLEND_STATE
            }
        }
    });

    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);
    return pipeline;
}

SDL_GPUShader *LoadSPIRVShader(SDL_GPUDevice *gpu, const char *shader_path) {
    SDL_ShaderCross_ShaderStage stage;
    if (SDL_strstr(shader_path, ".vert")) stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    else if (SDL_strstr(shader_path, ".frag")) stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_strstr() in LoadSPIRV(): Shader at path %s must include `.vert` or `.frag` to determine shader type.\n", shader_path);
        return NULL;
    }

    usize shader_size;
    const void *shader_code = SDL_LoadFile(shader_path, &shader_size);
    if (shader_code == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LoadFile() in LoadSPIRV(): Couldn't find shader at path %s.\n", shader_path);
        return NULL;
    }

    const SDL_ShaderCross_GraphicsShaderMetadata *metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(shader_code, shader_size, 0);
    return SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(gpu, &(SDL_ShaderCross_SPIRV_Info) {
        .bytecode = shader_code,
        .bytecode_size = shader_size,
        .entrypoint = "main",
        .shader_stage = stage,
    }, &metadata->resource_info, 0);
}

void UploadIntoGPUBuffers(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const UploadGPUBufferBinding *bindings, const usize num_bindings) {
    if (num_bindings == 0) return;
    u32 total_size = 0;
    for (usize i = 0; i < num_bindings; i++) {
        total_size += bindings[i].size;
    }

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &(SDL_GPUTransferBufferCreateInfo) {
        .size = total_size,
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD
    });

    u32 data_offset = 0;
    u8 *data = SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false);
    for (usize i = 0; i < num_bindings; i++) {
        SDL_memcpy(data + data_offset, bindings[i].source + bindings[i].source_offset, bindings[i].size);
        data_offset += bindings[i].size;
    }

    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    u32 buffer_offset = 0;
    for (usize i = 0; i < num_bindings; ++i) {
        SDL_UploadToGPUBuffer(
            copy_pass,
            &(SDL_GPUTransferBufferLocation) { .transfer_buffer = transfer_buffer, .offset = buffer_offset },
            &(SDL_GPUBufferRegion) { .buffer = bindings[i].buffer, .offset = bindings[i].buffer_offset, .size = bindings[i].size },
            false
        );

        buffer_offset += bindings[i].size;
    }

    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
}

GPUArray CreateGPUArray(SDL_GPUDevice *gpu, const u32 size) {
    const SDL_GPUBufferCreateInfo info = {
        .size = size,
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
    };

    return (GPUArray) {
        .buffer = SDL_CreateGPUBuffer(gpu, &info),
        .info = info,
        .used = 0,
    };
}

void ExpandGPUArray(GPUArray *array, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const u32 size) {
    if ((array->used + size) <= array->info.size) return;

    SDL_GPUBuffer *old_buffer = array->buffer;
    u32 old_size = array->info.size;
    array->info.size = (array->info.size + size) > 2 * array->info.size ? (array->info.size + size) : 2 * array->info.size;
    array->buffer = SDL_CreateGPUBuffer(gpu, &array->info);

    SDL_CopyGPUBufferToBuffer(
        copy_pass,
        &(SDL_GPUBufferLocation) { .buffer = old_buffer, .offset = 0 },
        &(SDL_GPUBufferLocation) { .buffer = array->buffer, .offset = 0 },
        old_size,
        false
    );

    SDL_ReleaseGPUBuffer(gpu, old_buffer);
}

void AppendGPUArrays(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const AppendGPUArrayBinding *bindings, const usize num_bindings) {
    UploadGPUBufferBinding *upload_bindings = SDL_malloc(sizeof(UploadGPUBufferBinding) * num_bindings);
    for (usize i = 0; i < num_bindings; i++) {
        ExpandGPUArray(bindings[i].array, gpu, copy_pass, bindings[i].size);
        upload_bindings[i] = (UploadGPUBufferBinding) {
            .buffer = bindings[i].array->buffer,
            .source = bindings[i].source,
            .size = bindings[i].size,
            .buffer_offset = bindings[i].array->used,
            .source_offset = bindings[i].source_offset
        };

        bindings[i].array->used += bindings[i].size;
    }

    UploadIntoGPUBuffers(gpu, copy_pass, upload_bindings, num_bindings);
    SDL_free(upload_bindings);
}

#endif
