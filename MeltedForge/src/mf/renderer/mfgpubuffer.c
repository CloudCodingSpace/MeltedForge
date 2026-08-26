#ifdef __cplusplus
extern "C" {
#endif

#include "mfgpubuffer.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/command_buffer.h"
#include "vk/buffer.h"
#include "vk/rendergraph.h"

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

void mfGpuBufferFree(MFGpuBuffer** _buffer) {
    MF_PANIC_IF(_buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    
    MFGpuBuffer* buffer = _buffer[0];
    
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");
    
    for(i32 i = 0; i < (buffer->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanBufferFree(&buffer->buffer[i]);

    MF_SETMEM(buffer, 0, sizeof(MFGpuBuffer));
    MF_FREEMEM(buffer);
    MF_SETMEM(_buffer, 0, sizeof(MFGpuBuffer*));
}

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    u32 idx = buffer->config.frameSynced ? buffer->backend->frameIndex : 0;
    VulkanBufferUploadData(&buffer->buffer[idx], data);
}

void* mfGpuBufferGetData(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(!buffer->init, mfGetLogger(), "The gpu buffer isn't initialised!");

    VulkanBackendCtx* ctx = buffer->ctx;
    VulkanBuffer* bufferBackend = &buffer->buffer[buffer->config.frameSynced ? buffer->backend->frameIndex : 0];

    if(buffer->config.frequentUpdates) {
        void* data = MF_ALLOCMEM(void, buffer->config.size);
        memcpy(data, bufferBackend->mappedMem, buffer->config.size);
        return data;
    }

    VulkanBuffer stagingBuffer = {};
    {
        VulkanBufferInfo info = {
            .ctx = ctx,
            .frequentUpdates = true,
            .pool = ctx->commandPool,
            .size = buffer->config.size,
            .type = VULKAN_BUFFER_TYPE_STAGING
        };
        VulkanBufferAllocate(&stagingBuffer, info);
    }

    VulkanCommandBufferBegin(bufferBackend->cmdBuff, true);

    VkBufferCopy region = {
        .size = buffer->config.size,
        .srcOffset = 0, // TODO: Make it configurable if required
        .dstOffset = 0
    };

    vkCmdCopyBuffer(bufferBackend->cmdBuff, bufferBackend->handle, stagingBuffer.handle, 1, &region);

    VulkanCommandBufferEnd(bufferBackend->cmdBuff);

    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &bufferBackend->cmdBuff
    };

    VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &info, bufferBackend->fence));
    VK_CHECK(vkWaitForFences(ctx->device, 1, &bufferBackend->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(ctx->device, 1, &bufferBackend->fence));
    VK_CHECK(vkResetCommandBuffer(bufferBackend->cmdBuff, 0));

    void* data = MF_ALLOCMEM(void, buffer->config.size);
    memcpy(data, stagingBuffer.mappedMem, buffer->config.size);

    VulkanBufferFree(&stagingBuffer);

    return data;
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
    if(buffer->backend->renderGraph != mfnull) {
        buff = buffer->backend->renderGraph->commandBuffers[buffer->backend->frameIndex];
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