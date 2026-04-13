#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ctx.h"

typedef struct VulkanRenderPassInfo_s {
    VkFormat format; 
    VkImageLayout initialLayout; 
    VkImageLayout finalLayout; 
    b8 hasDepth; 
    b8 renderTarget;
} VulkanRenderPassInfo;

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VulkanRenderPassInfo info);
void VulkanRenderPassDestroy(VulkanBackendCtx* ctx, VkRenderPass pass);

#ifdef __cplusplus
}
#endif