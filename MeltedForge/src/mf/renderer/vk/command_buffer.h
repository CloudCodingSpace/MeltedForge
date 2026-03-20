#pragma once

#include "ctx.h"

VkCommandPool VulkanCommandPoolCreate(VulkanBackendCtx* ctx, u32 queueIdx);
void VulkanCommandPoolDestroy(VulkanBackendCtx* ctx, VkCommandPool pool);

VkCommandBuffer VulkanCommandBufferAllocate(VulkanBackendCtx* ctx, VkCommandPool pool, b8 isPrimary);
void VulkanCommandBufferFree(VulkanBackendCtx* ctx, VkCommandBuffer buffer, VkCommandPool pool);
void VulkanCommandBufferBegin(VkCommandBuffer buffer);
void VulkanCommandBufferEnd(VkCommandBuffer buffer);