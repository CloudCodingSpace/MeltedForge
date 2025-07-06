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
    VkFramebuffer* fbs;
    VkRenderPass pass;
    VkDescriptorSet* descs;

    VkCommandBuffer buffs[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];

    b8 hasDepth;
};