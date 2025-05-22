#include "mfbuffer.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/buffer.h"

struct MFGpuBuffer_s {
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    VulkanBuffer buffer;
    MFGpuBufferConfig config;
};

void mfGpuBufferAllocate(MFGpuBuffer* buffer, MFGpuBufferConfig config, MFRenderer* renderer) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_ASSERT(config.type == MF_GPU_BUFFER_TYPE_NONE, mfGetLogger(), "The gpu buffer type can't be none!");

    buffer->config = config;
    buffer->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    buffer->ctx = &buffer->backend->ctx;

    if(config.size == 0)
        return;
    else {
        VulkanBufferAllocate(&buffer->buffer, buffer->ctx, buffer->ctx->cmdPool, config.size, config.data, (VulkanBufferTypes)(i32)config.type);
    }
}

void mfGpuBufferFree(MFGpuBuffer* buffer) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    
    VulkanBufferFree(&buffer->buffer, buffer->ctx);

    MF_SETMEM(buffer, 0, sizeof(MFGpuBuffer));
}

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    buffer->config.data = data;

    VulkanBufferUploadData(&buffer->buffer, buffer->ctx, buffer->ctx->cmdPool, data);
}

void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    buffer->config.data = data;
    buffer->config.size = size;
    
    VulkanBufferResize(&buffer->buffer, buffer->ctx, buffer->ctx->cmdPool, size, data);
}

void mfGpuBufferBind(MFGpuBuffer* buffer) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");

    VkCommandBuffer buff = buffer->backend->cmdBuffers[buffer->backend->crntFrmIdx];

    if(buffer->config.type == MF_GPU_BUFFER_TYPE_VERTEX) {
        VkDeviceSize offsets[] = { 0 }; // TODO: Make it configurable if necessary

        vkCmdBindVertexBuffers(buff, 0, 1, &buffer->buffer.handle, offsets);
    }
    else {
        vkCmdBindIndexBuffer(buff, buffer->buffer.handle, 0, VK_INDEX_TYPE_UINT32); // TODO: Make the offset configurable if necessary
    }
}

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");

    return &buffer->config;
}

size_t mfGpuBufferGetSizeInBytes() {
    return sizeof(MFGpuBuffer);
}