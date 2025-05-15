#pragma once

#include "window/mfwindow.h"

#include "ctx.h"
#include "common.h"

typedef struct VulkanBackend_s {
    VulkanBackendCtx ctx;
    u32 scImgIdx, crntFrmIdx;
    VkClearValue clearColor;

    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffers[FRAMES_IN_FLIGHT];

    VkRenderPass pass;
    u32 fbCount;
    VkFramebuffer* fbs;

    VkSemaphore imgAvailableSemas[FRAMES_IN_FLIGHT];
    VkSemaphore rndrFinishedSemas[FRAMES_IN_FLIGHT];
    VkFence inFlightFences[FRAMES_IN_FLIGHT];
} VulkanBackend;

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window);
void VulkanBckndShutdown(VulkanBackend* backend);

void VulkanBckndBeginframe(VulkanBackend* backend, MFWindow* window);
void VulkanBckndEndframe(VulkanBackend* backend, MFWindow* window);

void VulkanBackendDrawVertices(VulkanBackend* backend, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance);