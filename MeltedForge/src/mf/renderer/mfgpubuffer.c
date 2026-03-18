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
};

void mfGpuBufferAllocate(MFGpuBuffer* buffer, MFGpuBufferConfig config, MFRenderer* renderer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(config.type == MF_GPU_BUFFER_TYPE_NONE, mfGetLogger(), "The gpu buffer type can't be none!");

    buffer->config = config;
    buffer->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    buffer->ctx = &buffer->backend->ctx;

    if(config.size == 0)
        return;
    else {
        for(u32 i = 0; i < ((config.type == MF_GPU_BUFFER_TYPE_UBO) ? FRAMES_IN_FLIGHT : 1); i++)
            VulkanBufferAllocate(&buffer->buffer[i], buffer->ctx, buffer->ctx->cmdPool, config.size, config.data, (VulkanBufferTypes)(i32)config.type);
    }
}

void mfGpuBufferFree(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    
    
    for(u32 i = 0; i < ((buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanBufferFree(&buffer->buffer[i], buffer->ctx);

    MF_SETMEM(buffer, 0, sizeof(MFGpuBuffer));
}

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    buffer->config.data = data;

    u32 idx = (buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) ? buffer->backend->crntFrmIdx : 0;
    VulkanBufferUploadData(&buffer->buffer[idx], buffer->ctx, buffer->ctx->cmdPool, data);
}

void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    buffer->config.data = data;
    buffer->config.size = size;

    for(u32 i = 0; i < ((buffer->config.type == MF_GPU_BUFFER_TYPE_UBO) ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanBufferResize(&buffer->buffer[i], buffer->ctx, buffer->ctx->cmdPool, size, data);
}

void mfGpuBufferBind(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");

    if(buffer->config.type == MF_GPU_BUFFER_TYPE_UBO)
        return;

    VkCommandBuffer buff = buffer->backend->cmdBuffers[buffer->backend->crntFrmIdx];
    if(buffer->backend->rt != mfnull) {
        buff = buffer->backend->rt->buffs[buffer->backend->crntFrmIdx];
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

    return &buffer->config;
}

size_t mfGpuBufferGetSizeInBytes(void) {
    return sizeof(MFGpuBuffer);
}

MFResourceDesc mfGpuBufferGetDescription(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    
    return (MFResourceDesc) {
        .binding = buffer->config.binding,
        .descriptorCount = 1,
        .descriptorType = MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER,
        .stageFlags = buffer->config.stage
    };
}

struct VulkanBuffer_s* mfGpuBufferGetBackend(MFGpuBuffer* buffer) {
    MF_PANIC_IF(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");

    return buffer->buffer;
}