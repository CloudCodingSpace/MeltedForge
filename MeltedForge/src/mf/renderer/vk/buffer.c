#ifdef __cplusplus
extern "C" {
#endif

#include "buffer.h"

#include "command_buffer.h"

void staging_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .size = buffer->size
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_ONLY
    };

    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));
}

void ubo_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .size = buffer->size
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));
    VK_CHECK(vmaMapMemory(ctx->vmaAllocator, buffer->allocation, &buffer->mappedMem));

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->size);
}

void vertex_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->size
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };
    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));

    if(buffer->data) {
        VulkanBufferUploadData(buffer, ctx, pool, buffer->data);
    }
    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->size);
}

void index_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->size
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };
    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));

    if(buffer->data) {
        VulkanBufferUploadData(buffer, ctx, pool, buffer->data);
    }

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->size);
}

void VulkanBufferAllocate(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, u64 size, void* data, VulkanBufferTypes type) {
    buffer->type = type;
    buffer->data = data;
    buffer->size = size;

    if(type == VULKAN_BUFFER_TYPE_VERTEX) {
        vertex_buff(buffer, ctx, pool);
    }
    else if(type == VULKAN_BUFFER_TYPE_INDEX) {
        index_buff(buffer, ctx, pool);
    }
    else if(type == VULKAN_BUFFER_TYPE_UBO) {
        ubo_buff(buffer, ctx);
    }
    else if(type == VULKAN_BUFFER_TYPE_STAGING) {
        staging_buff(buffer, ctx);
    }
}

void VulkanBufferFree(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    if(buffer->type == VULKAN_BUFFER_TYPE_UBO)
        vmaUnmapMemory(ctx->vmaAllocator, buffer->allocation);

    vmaDestroyBuffer(ctx->vmaAllocator, buffer->handle, buffer->allocation);

    if(buffer->type != VULKAN_BUFFER_TYPE_STAGING)
        MF_INFO(mfGetLogger(), "(From the vulkan backend) Freed a buffer of size: %zu bytes", buffer->size);

    MF_SETMEM(buffer, 0, sizeof(VulkanBuffer));
}

void VulkanBufferUploadData(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, void* data) {
    buffer->data = data;
    
    if(buffer->type == VULKAN_BUFFER_TYPE_UBO) {
        memcpy(buffer->mappedMem, data, buffer->size);
        return;
    }

    VulkanBuffer staging = {
        .data = buffer->data,
        .size = buffer->size,
        .type = VULKAN_BUFFER_TYPE_STAGING
    };

    staging_buff(&staging, ctx);

    void* mappedMem = mfnull;
    VK_CHECK(vmaMapMemory(ctx->vmaAllocator, staging.allocation, &mappedMem));
    memcpy(mappedMem, staging.data, staging.size);
    vmaUnmapMemory(ctx->vmaAllocator, staging.allocation);

    // Copy
    {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkCommandBuffer buff = VulkanCommandBufferAllocate(ctx, pool, true);

        VulkanCommandBufferBegin(buff);

        VkBufferCopy region = {
            .size = staging.size,
            .dstOffset = 0, // NOTE: Make the offset configurable if necessary
            .srcOffset = 0 // NOTE: Make the offset configurable if necessary
        };

        vkCmdCopyBuffer(buff, staging.handle, buffer->handle, 1, &region);

        VulkanCommandBufferEnd(buff);

        VkFence fence;
        {
            VkFenceCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
            };

            VK_CHECK(vkCreateFence(ctx->device, &info, ctx->allocator, &fence));
        }

        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &buff
        };

        VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &info, fence));
        VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(ctx->device, 1, &fence));

        vkDestroyFence(ctx->device, fence, ctx->allocator);
        VulkanCommandBufferFree(ctx, buff, pool);
    }

    VulkanBufferFree(&staging, ctx);
}

void VulkanBufferResize(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, u64 size, void* data) {
    VulkanBufferFree(buffer, ctx);

    VulkanBufferAllocate(buffer, ctx, pool, size, data, buffer->type);
}

#ifdef __cplusplus
}
#endif