#pragma once

#include <vulkan/vulkan.h>

#include "core/mfutils.h"

struct VulkanBackendCtx_s;

typedef struct VulkanImage_s {
    VkImage image;
    VkImageView view;
    VkDeviceMemory mem;

    VkFormat format;
    u32 width, height;
} VulkanImage;

void VulkanImageCreate(VulkanImage* image, struct VulkanBackendCtx_s* ctx, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlagBits usage, VkImageAspectFlags aspectFlags, VkMemoryPropertyFlags memFlags);
void VulkanImageDestroy(VulkanImage* image, struct VulkanBackendCtx_s* ctx);