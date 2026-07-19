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
    buffer->config.data = mfnull;
    buffer->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    buffer->ctx = &buffer->backend->ctx;

    VulkanBufferInfo info = {
        .ctx = buffer->ctx,
        .pool = buffer->ctx->commandPool,
        .data = config.data,
        .size = config.size,
        .type = (VulkanBufferTypes)(i32)config.type,
        .frequentUpdates = config.frequentUpdates
    };

    for(i32 i = 0; i < (config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanBufferAllocate(&buffer->buffer[i], info);

    buffer->init = true;
    return buffer;
}

void mfGpuBufferFree(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");
    
    for(i32 i = 0; i < (buffer->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanBufferFree(&buffer->buffer[i]);

    MF_SETMEM(buffer, 0, sizeof(MFGpuBuffer));
    MF_FREEMEM(buffer);
}

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    u32 idx = buffer->config.frameSynced ? buffer->backend->frameIndex : 0;
    VulkanBufferUploadData(&buffer->buffer[idx], data);
}

void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    buffer->config.data = data;
    buffer->config.size = size;

    for(i32 i = 0; i < (buffer->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++) {
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
    
    u32 idx = buffer->config.frameSynced ? buffer->backend->frameIndex : 0;

    VkCommandBuffer buff = buffer->backend->commandBuffers[buffer->backend->frameIndex];
    if(buffer->backend->renderTarget != mfnull) {
        buff = buffer->backend->renderTarget->commandBuffers[buffer->backend->frameIndex];
    }

    if(buffer->config.type == MF_GPU_BUFFER_TYPE_VERTEX) {
        VkDeviceSize offsets[] = { 0 }; // NOTE: Make it configurable if necessary

        vkCmdBindVertexBuffers(buff, 0, 1, &buffer->buffer[idx].handle, offsets);
    }
    else if (buffer->config.type == MF_GPU_BUFFER_TYPE_INDEX) {
        vkCmdBindIndexBuffer(buff, buffer->buffer[idx].handle, 0, VK_INDEX_TYPE_UINT32); // NOTE: Make the offset configurable if necessary
    }
}

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    return &buffer->config;
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

size_t mfGpuBufferGetSizeInBytes(void) {
    return sizeof(MFGpuBuffer);
}

#ifdef __cplusplus
}
#endif