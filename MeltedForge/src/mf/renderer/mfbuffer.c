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
        VulkanBufferAllocate(&buffer->buffer, buffer->ctx, buffer->backend->cmdPool, config.size, config.data, (VulkanBufferTypes)(i32)config.type);
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

    VulkanBufferUploadData(&buffer->buffer, buffer->ctx, buffer->backend->cmdPool, data);
}

void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");
    buffer->config.data = data;
    buffer->config.size = size;

    VulkanBufferResize(&buffer->buffer, buffer->ctx, buffer->backend->cmdPool, size, data);
}

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer) {
    MF_ASSERT(buffer == mfnull, mfGetLogger(), "The buffer handle provided shouldn't be null!");

    return &buffer->config;
}

size_t mfGpuBufferGetSizeInBytes() {
    return sizeof(MFGpuBuffer);
}