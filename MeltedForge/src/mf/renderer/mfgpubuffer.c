#ifdef __cplusplus
extern "C" {
#endif

#include "mfgpubuffer.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/buffer.h"
#include "vk/render_target.h"

struct MFGpuBuffer_s {
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    VulkanBuffer buffer[FRAMES_IN_FLIGHT];
    MFGpuBufferConfig config;
    bool init;
};

MFGpuBuffer* mfGpuBufferAllocate(MFGpuBufferConfig config, MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(config.type == MF_GPU_BUFFER_TYPE_NONE, mfGetLogger(), "The gpu buffer type can't be none!");

    MFGpuBuffer* buffer = MF_ALLOCMEM(MFGpuBuffer, sizeof(MFGpuBuffer));

    buffer->config = config;
    buffer->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    buffer->ctx = &buffer->backend->ctx;

    VulkanBufferInfo info = {
        .ctx = buffer->ctx,
        .pool = buffer->ctx->commandPool,
        .data = config.data,
        .size = config.size,
        .type = (VulkanBufferTypes)(i32)config.type
    };

    if(config.size == 0)
        return buffer;
    else {
        bool isShaderRes = (config.type == MF_GPU_BUFFER_TYPE_UBO) || (buffer->config.type == MF_GPU_BUFFER_TYPE_SSBO);
        for(i32 i = 0; i < (isShaderRes ? FRAMES_IN_FLIGHT : 1); i++)
            VulkanBufferAllocate(&buffer->buffer[i], info);
    }

    buffer->init = true;
    return buffer;
}

void mfGpuBufferFree(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");
    
    bool isShaderRes = (buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) || (buffer->config.type == MF_GPU_BUFFER_TYPE_SSBO);
    for(i32 i = 0; i < (isShaderRes ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanBufferFree(&buffer->buffer[i]);

    MF_SETMEM(buffer, 0, sizeof(MFGpuBuffer));
    MF_FREEMEM(buffer);
}

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    buffer->config.data = data;

    bool isShaderRes = (buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) || (buffer->config.type == MF_GPU_BUFFER_TYPE_SSBO);
    VulkanBufferUploadData(&buffer->buffer[isShaderRes ? buffer->backend->frameIndex : 0], data);
}

void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    buffer->config.data = data;
    buffer->config.size = size;

    bool isShaderRes = (buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) || (buffer->config.type == MF_GPU_BUFFER_TYPE_SSBO);
    for(i32 i = 0; i < (isShaderRes ? FRAMES_IN_FLIGHT : 1); i++) {
        VulkanBufferResize(&buffer->buffer[i], size);
        if(data != mfnull)
            VulkanBufferUploadData(&buffer->buffer[i], data);
    }
}

void mfGpuBufferBind(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    bool isShaderRes = (buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) || (buffer->config.type == MF_GPU_BUFFER_TYPE_SSBO);
    if(isShaderRes)
        return;

    VkCommandBuffer buff = buffer->backend->commandBuffers[buffer->backend->frameIndex];
    if(buffer->backend->renderTarget != mfnull) {
        buff = buffer->backend->renderTarget->commandBuffers[buffer->backend->frameIndex];
    }

    if(buffer->config.type == MF_GPU_BUFFER_TYPE_VERTEX) {
        VkDeviceSize offsets[] = { 0 }; // NOTE: Make it configurable if necessary

        vkCmdBindVertexBuffers(buff, 0, 1, &buffer->buffer[0].handle, offsets);
    }
    else if (buffer->config.type == MF_GPU_BUFFER_TYPE_INDEX) {
        vkCmdBindIndexBuffer(buff, buffer->buffer[0].handle, 0, VK_INDEX_TYPE_UINT32); // NOTE: Make the offset configurable if necessary
    }
}

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    return &buffer->config;
}

size_t mfGpuBufferGetSizeInBytes(void) {
    return sizeof(MFGpuBuffer);
}

MFResourceDescription mfGpuBufferGetDescription(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");
    
    MFResourceDescriptionType type = MF_RES_DESCRIPTION_TYPE_MAX_ENUM;
    if(buffer->config.type == MF_GPU_BUFFER_TYPE_UBO)
        type = MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER;
    else if(buffer->config.type == MF_GPU_BUFFER_TYPE_SSBO)
        type = MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER;

    return (MFResourceDescription) {
        .descriptorCount = 1,
        .descriptorType = type,
        .stageFlags = buffer->config.stage
    };
}

void* mfGpuBufferGetBackend(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    return buffer->buffer;
}

#ifdef __cplusplus
}
#endif