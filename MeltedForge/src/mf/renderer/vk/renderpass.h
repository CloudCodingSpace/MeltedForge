#pragma once

#include "ctx.h"

typedef struct VulkanRenderPassInfo_s {
    VkFormat format; 
    VkImageLayout initiaLay; 
    VkImageLayout finalLay; 
    b8 hasDepth; 
    b8 renderTarget;
} VulkanRenderPassInfo;

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VulkanRenderPassInfo info);
void VulkanRenderPassDestroy(VulkanBackendCtx* ctx, VkRenderPass pass);