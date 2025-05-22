#pragma once

#include <vulkan/vulkan.h>

#include "core/mfutils.h"

struct VulkanBackendCtx_s;

typedef struct VulkanImage_s {
    VkImage image;
    VkImageView view;
    VkDeviceMemory mem;
    VkSampler sampler;

    VkFormat format;
    u32 width, height;
    b8 gpuResource;
    u8* pixels;
} VulkanImage;

void VulkanImageCreate(VulkanImage* image, struct VulkanBackendCtx_s* ctx, u32 width, u32 height, b8 gpuResource, u8* pixels, VkFormat format, VkImageTiling tiling, VkImageUsageFlagBits usage, VkImageAspectFlags aspectFlags, VkMemoryPropertyFlags memFlags);
void VulkanImageDestroy(VulkanImage* image, struct VulkanBackendCtx_s* ctx);

void VulkanImageSetPixels(VulkanImage* image, struct VulkanBackendCtx_s* ctx, u8* pixels);