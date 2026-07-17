#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fb.h"

struct VulkanBackend_s;

typedef struct VulkanRenderPassInfo_s {
    VkFormat format; 
    VkImageLayout initialLayout, initialDepthLayout; 
    VkImageLayout finalLayout, finalDepthLayout;
    bool hasDepth;
    bool hasMsaa;
} VulkanRenderPassInfo;

typedef struct VulkanRenderPassBeginInfo_s {
    VulkanFramebuffer* fb;
    VkRect2D extent;
    u32 clearValueCount;
    VkClearValue* clearValues;
    VkCommandBuffer cmdBuff;
} VulkanRenderPassBeginInfo;

typedef struct VulkanRenderPass_s {
    VkRenderPass handle;
    struct VulkanBackend_s* backend;
    VulkanBackendCtx* ctx;
    VulkanRenderPassInfo info;
    bool begun;
} VulkanRenderPass;

void VulkanRenderPassCreate(VulkanRenderPass* pass, struct VulkanBackend_s* backend, VulkanRenderPassInfo info);
void VulkanRenderPassDestroy(VulkanRenderPass* pass);

void VulkanRenderPassBegin(VulkanRenderPass* pass, VulkanRenderPassBeginInfo info);
void VulkanRenderPassEnd(VulkanRenderPass* pass, VkCommandBuffer cmdBuff, VulkanFramebuffer* fb);

#ifdef __cplusplus
}
#endif