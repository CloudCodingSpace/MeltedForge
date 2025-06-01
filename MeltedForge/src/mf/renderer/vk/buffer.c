#include "buffer.h"

#include "cmd.h"

void staging_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .size = buffer->size
    };

    VK_CHECK(vkCreateBuffer(ctx->device, &info, ctx->allocator, &buffer->handle));

    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(ctx->device, buffer->handle, &req);

    VkMemoryAllocateInfo memInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = req.size,
        .memoryTypeIndex = FindMemoryType(ctx->physicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    };

    VK_CHECK(vkAllocateMemory(ctx->device, &memInfo, ctx->allocator, &buffer->mem));
    VK_CHECK(vkBindBufferMemory(ctx->device, buffer->handle, buffer->mem, 0)); // TODO: Make the offset configurable if necessary
}

void ubo_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .size = buffer->size
    };

    VK_CHECK(vkCreateBuffer(ctx->device, &info, ctx->allocator, &buffer->handle));

    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(ctx->device, buffer->handle, &req);

    VkMemoryAllocateInfo memInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = req.size,
        .memoryTypeIndex = FindMemoryType(ctx->physicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    };

    VK_CHECK(vkAllocateMemory(ctx->device, &memInfo, ctx->allocator, &buffer->mem));
    VK_CHECK(vkBindBufferMemory(ctx->device, buffer->handle, buffer->mem, 0)); // TODO: Make the offset configurable if necessary

    vkMapMemory(ctx->device, buffer->mem, 0, buffer->size, 0, &buffer->mappedMem);  // TODO: Make the offset configurable if necessary
}

void vertex_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->size
    };

    VK_CHECK(vkCreateBuffer(ctx->device, &info, ctx->allocator, &buffer->handle));

    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(ctx->device, buffer->handle, &req);

    VkMemoryAllocateInfo memInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = req.size,
        .memoryTypeIndex = FindMemoryType(ctx->physicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(ctx->device, &memInfo, ctx->allocator, &buffer->mem));
    VK_CHECK(vkBindBufferMemory(ctx->device, buffer->handle, buffer->mem, 0)); // TODO: Make the offset configurable if necessary

    if(buffer->data) {
        VulkanBufferUploadData(buffer, ctx, pool, buffer->data);
    }
}

void index_buff(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer->size
    };

    VK_CHECK(vkCreateBuffer(ctx->device, &info, ctx->allocator, &buffer->handle));

    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(ctx->device, buffer->handle, &req);

    VkMemoryAllocateInfo memInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = req.size,
        .memoryTypeIndex = FindMemoryType(ctx->physicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(ctx->device, &memInfo, ctx->allocator, &buffer->mem));
    VK_CHECK(vkBindBufferMemory(ctx->device, buffer->handle, buffer->mem, 0)); // TODO: Make the offset configurable if necessary

    if(buffer->data) {
        VulkanBufferUploadData(buffer, ctx, pool, buffer->data);
    }
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
        vkUnmapMemory(ctx->device, buffer->mem);

    vkDestroyBuffer(ctx->device, buffer->handle, ctx->allocator);
    vkFreeMemory(ctx->device, buffer->mem, ctx->allocator);

    buffer->handle = 0;
    buffer->mem = 0;
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
    VK_CHECK(vkMapMemory(ctx->device, staging.mem, 0, buffer->size, 0, &mappedMem));

    memcpy(mappedMem, staging.data, staging.size);

    vkUnmapMemory(ctx->device, staging.mem);

    // Copy
    {
        VkCommandBuffer buff = VulkanCommandBufferAllocate(ctx, pool, true);

        VulkanCommandBufferBegin(buff);

        VkBufferCopy region = {
            .size = staging.size,
            .dstOffset = 0, // TODO: Make the offset configurable if necessary
            .srcOffset = 0 // TODO: Make the offset configurable if necessary
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

        VK_CHECK(vkQueueSubmit(ctx->qData.tQueue, 1, &info, fence));
        VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(ctx->device, fence, ctx->allocator);
        VulkanCommandBufferFree(ctx, buff, pool);
    }

    VulkanBufferFree(&staging, ctx);
}

void VulkanBufferResize(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, u64 size, void* data) {
    VulkanBufferFree(buffer, ctx);

    VulkanBufferAllocate(buffer, ctx, pool, size, data, buffer->type);
}