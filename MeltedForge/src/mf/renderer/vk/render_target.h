#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct VulkanBackend_s;

struct MFRenderTarget_s {
    void* renderer;
    struct VulkanBackend_s* backend;

    void* userData;
    void (*resizeCallback)(void* userData);
    VulkanImage depthImage;
    VulkanImage images[FRAMES_IN_FLIGHT];
    VkFramebuffer frameBuffers[FRAMES_IN_FLIGHT];
    VkRenderPass renderPass;
    VkDescriptorSet igSets[FRAMES_IN_FLIGHT];

    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
    VkSemaphore* renderFinishedSemas;

    bool hasDepth, init;
};

#ifdef __cplusplus
}
#endif