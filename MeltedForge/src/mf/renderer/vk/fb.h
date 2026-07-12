#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ctx.h"

typedef struct VulkanFramebuffer_s {
    VkFramebuffer buffer;
    VulkanBackendCtx* ctx;
    VkRenderPass pass;
    u32 attachmentCount;
    VulkanImage* attachments;
    VkExtent2D extent;
} VulkanFramebuffer;

void VulkanFramebufferCreate(VulkanFramebuffer* fb, VulkanBackendCtx* ctx, VkRenderPass pass, u32 attachmentCount, VulkanImage* attachments, VkExtent2D extent);
void VulkanFramebufferDestroy(VulkanFramebuffer* fb);

#ifdef __cplusplus
}
#endif