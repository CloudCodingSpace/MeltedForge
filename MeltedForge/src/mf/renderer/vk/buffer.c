#ifdef __cplusplus
extern "C" {
#endif

#include "buffer.h"

#include "command_buffer.h"

void staging_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->info.size,
        .queueFamilyIndexCount = ctx->uniqueQueueCount,
        .pQueueFamilyIndices = ctx->uniqueQueues
    };

    if(ctx->uniqueQueueCount > 1) {
        info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_ONLY
    };

    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));
}

void ubo_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->info.size,
        .queueFamilyIndexCount = ctx->uniqueQueueCount,
        .pQueueFamilyIndices = ctx->uniqueQueues
    };

    if(ctx->uniqueQueueCount > 1) {
        info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    if(buffer->info.frequentUpdates)
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));    
    if(buffer->info.frequentUpdates)
        VK_CHECK(vmaMapMemory(ctx->vmaAllocator, buffer->allocation, &buffer->mappedMem));

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->info.size);
}

void ssbo_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->info.size,
        .queueFamilyIndexCount = ctx->uniqueQueueCount,
        .pQueueFamilyIndices = ctx->uniqueQueues
    };

    if(ctx->uniqueQueueCount > 1) {
        info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    if(buffer->info.frequentUpdates) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));
    if(buffer->info.frequentUpdates)
        VK_CHECK(vmaMapMemory(ctx->vmaAllocator, buffer->allocation, &buffer->mappedMem));

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->info.size);
}

void vertex_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->info.size,
        .queueFamilyIndexCount = ctx->uniqueQueueCount,
        .pQueueFamilyIndices = ctx->uniqueQueues
    };
    
    if(ctx->uniqueQueueCount > 1) {
        info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };
    
    if(buffer->info.frequentUpdates) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }
    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));
    if(buffer->info.frequentUpdates)
        VK_CHECK(vmaMapMemory(ctx->vmaAllocator, buffer->allocation, &buffer->mappedMem));

    if(buffer->info.data) {
        VulkanBufferUploadData(buffer, buffer->info.data);
    }
    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->info.size);
}

void index_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->info.size,
        .queueFamilyIndexCount = ctx->uniqueQueueCount,
        .pQueueFamilyIndices = ctx->uniqueQueues
    };

    if(ctx->uniqueQueueCount > 1) {
        info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };
    if(buffer->info.frequentUpdates) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    VK_CHECK(vmaCreateBuffer(ctx->vmaAllocator, &info, &allocInfo, &buffer->handle, &buffer->allocation, mfnull));
    if(buffer->info.frequentUpdates)
        VK_CHECK(vmaMapMemory(ctx->vmaAllocator, buffer->allocation, &buffer->mappedMem));

    if(buffer->info.data) {
        VulkanBufferUploadData(buffer, buffer->info.data);
    }

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Allocated a buffer of size: %zu bytes", buffer->info.size);
}

void VulkanBufferAllocate(VulkanBuffer* buffer, VulkanBufferInfo info) {
    buffer->info = info;

    if(info.type == VULKAN_BUFFER_TYPE_VERTEX) {
        vertex_buff(buffer, info.ctx, info.pool);
    }
    else if(info.type == VULKAN_BUFFER_TYPE_INDEX) {
        index_buff(buffer, info.ctx, info.pool);
    }
    else if(info.type == VULKAN_BUFFER_TYPE_UBO) {
        ubo_buff(buffer, info.ctx);
    }
    else if(info.type == VULKAN_BUFFER_TYPE_STAGING) {
        staging_buff(buffer, info.ctx);
    }
    else if(info.type == VULKAN_BUFFER_TYPE_SSBO) {
        ssbo_buff(buffer, info.ctx);
    }
}

void VulkanBufferFree(VulkanBuffer* buffer) {
    VulkanBackendCtx* ctx = buffer->info.ctx;

    if(buffer->info.frequentUpdates)
        vmaUnmapMemory(ctx->vmaAllocator, buffer->allocation);

    vmaDestroyBuffer(ctx->vmaAllocator, buffer->handle, buffer->allocation);

    if(buffer->info.type != VULKAN_BUFFER_TYPE_STAGING)
        MF_INFO(mfGetLogger(), "(From the vulkan backend) Freed a buffer of size: %zu bytes", buffer->info.size);

    MF_SETMEM(buffer, 0, sizeof(VulkanBuffer));
}

void VulkanBufferUploadData(VulkanBuffer* buffer, void* data) {
    buffer->info.data = data;
    VulkanBackendCtx* ctx = buffer->info.ctx;
    
    if(buffer->info.frequentUpdates) {
        memcpy(buffer->mappedMem, data, buffer->info.size);
        return;
    }

    VulkanBuffer staging = {
        .info = {
            .data = buffer->info.data,
            .size = buffer->info.size,
            .type = VULKAN_BUFFER_TYPE_STAGING,
            .ctx = ctx
        }
    };

    staging_buff(&staging, buffer->info.ctx);

    void* mappedMem = mfnull;
    VK_CHECK(vmaMapMemory(ctx->vmaAllocator, staging.allocation, &mappedMem));
    memcpy(mappedMem, staging.info.data, staging.info.size);
    vmaUnmapMemory(ctx->vmaAllocator, staging.allocation);

    // Copy
    {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkCommandBuffer buff = VulkanCommandBufferAllocate(ctx, buffer->info.pool, true);

        VulkanCommandBufferBegin(buff, true);

        VkBufferCopy region = {
            .size = staging.info.size,
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
        VulkanCommandBufferFree(ctx, buff, buffer->info.pool);
    }

    VulkanBufferFree(&staging);
}

void VulkanBufferResize(VulkanBuffer* buffer, u64 newSize) {
    VulkanBufferInfo info = buffer->info;
    VulkanBufferFree(buffer);

    info.size = newSize;
    VulkanBufferAllocate(buffer, info);
}

#ifdef __cplusplus
}
#endif