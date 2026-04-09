#pragma once

#include "common.h"

struct VulkanBackend_s;

struct MFRenderTarget_s {
    void* renderer;
    struct VulkanBackend_s* backend;

    void* userData;
    void (*resizeCallback)(void* userData);
    VulkanImage depthImage;
    VulkanImage* images;
    VkFramebuffer* frameBuffers;
    VkRenderPass renderPass;
    VkDescriptorSet* sets;

    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
    VkSemaphore* renderFinishedSemas;

    b8 hasDepth, init;
};