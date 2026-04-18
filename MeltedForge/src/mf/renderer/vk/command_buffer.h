#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ctx.h"

VkCommandPool VulkanCommandPoolCreate(VulkanBackendCtx* ctx, u32 queueIdx);
void VulkanCommandPoolDestroy(VulkanBackendCtx* ctx, VkCommandPool pool);

VkCommandBuffer VulkanCommandBufferAllocate(VulkanBackendCtx* ctx, VkCommandPool pool, bool isPrimary);
void VulkanCommandBufferFree(VulkanBackendCtx* ctx, VkCommandBuffer buffer, VkCommandPool pool);
void VulkanCommandBufferBegin(VkCommandBuffer buffer);
void VulkanCommandBufferEnd(VkCommandBuffer buffer);

#ifdef __cplusplus
}
#endif