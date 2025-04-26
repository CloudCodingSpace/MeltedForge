#pragma once

#include "window/mfwindow.h"

#include "ctx.h"

typedef struct VulkanBackend_s {
    VulkanBackendCtx ctx;
    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffer;
    VkRenderPass pass;
    u32 fbCount;
    VkFramebuffer* fbs;
} VulkanBackend;

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window);
void VulkanBckndShutdown(VulkanBackend* backend);

void VulkanBckndBeginframe(VulkanBackend* backend);
void VulkanBckndEndframe(VulkanBackend* backend);