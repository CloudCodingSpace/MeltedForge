#include "cmd.h"

#include "common.h"

VkCommandPool VulkanCommandPoolCreate(VulkanBackendCtx* ctx, u32 queueIdx) {
    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueIdx
    };

    VkCommandPool pool;
    VK_CHECK(vkCreateCommandPool(ctx->device, &info, ctx->allocator, &pool));

    return pool;
}

void VulkanCommandPoolDestroy(VulkanBackendCtx* ctx, VkCommandPool pool) {
    vkDestroyCommandPool(ctx->device, pool, ctx->allocator);
}

VkCommandBuffer VulkanCommandBufferAllocate(VulkanBackendCtx* ctx, VkCommandPool pool, b8 isPrimary) {
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = 1,
        .commandPool = pool,
        .level = (isPrimary) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY
    };

    VkCommandBuffer buffer;
    VK_CHECK(vkAllocateCommandBuffers(ctx->device, &info, &buffer));

    return buffer;
}

void VulkanCommandBufferFree(VulkanBackendCtx* ctx, VkCommandBuffer buffer, VkCommandPool pool) {
    vkFreeCommandBuffers(ctx->device, pool, 1, &buffer);
}