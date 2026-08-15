#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "gpu_res.h"
#include "fb.h"
#include "renderpass.h"

#include <cimgui_impl.h>

struct VulkanBackend_s;

typedef struct VulkanRenderTarget_s {
    void* renderer;
    struct VulkanBackend_s* backend;

    void* userData;
    void (*resizeCallback)(void* userData);
    VulkanImage depthImages[FRAMES_IN_FLIGHT];
    VulkanImage msaaImages[FRAMES_IN_FLIGHT];
    VulkanImage images[FRAMES_IN_FLIGHT];
    VulkanFramebuffer frameBuffers[FRAMES_IN_FLIGHT];
    VulkanRenderPass renderPass;
    VkSampleCountFlagBits samples;
    VkDescriptorSet igColorSets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];

    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
    VkSemaphore* renderFinishedSemas;
    VkFence fences[FRAMES_IN_FLIGHT];
    
    VkClearValue clearValue;

    bool hasDepth,hasMsaa;
} VulkanRenderTarget;

void VulkanRenderTargetCreate(VulkanRenderTarget* renderTarget, MFRenderer* renderer, bool hasDepth);
void VulkanRenderTargetDestroy(VulkanRenderTarget* renderTarget);

void VulkanRenderTargetResize(VulkanRenderTarget* renderTarget, MFVec2 extent);

void VulkanRenderTargetBegin(VulkanRenderTarget* renderTarget);
void VulkanRenderTargetEnd(VulkanRenderTarget* renderTarget, bool waitOnCpu);

#ifdef __cplusplus
}
#endif