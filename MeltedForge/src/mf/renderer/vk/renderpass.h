#pragma once

#include "ctx.h"

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VkFormat format, VkImageLayout initiaLay, VkImageLayout finalLay, b8 hasDepth);
void VulkanRenderPassDestroy(VulkanBackendCtx* ctx, VkRenderPass pass);