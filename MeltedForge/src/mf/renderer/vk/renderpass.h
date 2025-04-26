#pragma once

#include "ctx.h"

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VkFormat format, VkImageLayout initiaLay, VkImageLayout finalLay);
void VulkanRenderPassDestroy(VulkanBackendCtx* ctx, VkRenderPass pass);