#pragma once

#include "ctx.h"

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VkFormat format, VkImageLayout initiaLay, VkImageLayout finalLay, b8 hasDepth, b8 renderTarget);
void VulkanRenderPassDestroy(VulkanBackendCtx* ctx, VkRenderPass pass);