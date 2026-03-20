#pragma once

#include "window/mfwindow.h"

#include "ctx.h"
#include "common.h"

struct MFRenderTarget_s;

typedef struct VulkanBackendConfig_s {
    b8 enableUI;
    b8 enableDepth;
    b8 vsync;
    MFWindow* window;
    const char* appName;
} VulkanBackendConfig;

typedef struct VulkanBackend_s {
    VulkanBackendCtx ctx;
    u32 swapchainImageIndex, frameIndex;
    VkClearValue clearColor;
    b8 enableUI;
    b8 enableDepth;

    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];

    VkRenderPass pass;
    u32 frameBufferCount;
    VkFramebuffer* frameBuffers;

    VkSemaphore imageAvailableSemas[FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemas[FRAMES_IN_FLIGHT];
    VkFence inFlightFences[FRAMES_IN_FLIGHT];

    struct MFRenderTarget_s* renderTarget;
} VulkanBackend;

void VulkanBackendInit(VulkanBackend* backend, VulkanBackendConfig* config);
void VulkanBackendShutdown(VulkanBackend* backend);

void VulkanBackendBeginframe(VulkanBackend* backend, MFWindow* window);
void VulkanBackendEndframe(VulkanBackend* backend, MFWindow* window);

void VulkanBackendDrawVertices(VulkanBackend* backend, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance);
void VulkanBackendDrawVerticesIndexed(VulkanBackend* backend, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance);