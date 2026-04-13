#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ctx.h"

VkFramebuffer VulkanFbCreate(VulkanBackendCtx* ctx, VkRenderPass pass, u32 imgViewCount, VkImageView* imgViews, VkExtent2D extent);
void VulkanFbDestroy(VulkanBackendCtx* ctx, VkFramebuffer fb);

#ifdef __cplusplus
}
#endif