#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "gpu_res.h"
#include "fb.h"
#include "renderpass.h"

struct VulkanBackend_s;

struct MFRenderTarget_s {
    void* renderer;
    struct VulkanBackend_s* backend;

    void* userData;
    void (*resizeCallback)(void* userData);
    VulkanImage depthImage;
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

    bool hasDepth, init, begun, hasMsaa;
};

#ifdef __cplusplus
}
#endif