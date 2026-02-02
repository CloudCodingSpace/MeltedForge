#pragma once

#include <vulkan/vulkan.h>

#include "core/mfutils.h"

struct VulkanBackendCtx_s;

typedef struct VulkanImageInfo_s {
    u32 width, height;
    b8 gpuResource;
    u8* pixels;
    
    struct VulkanBackendCtx_s* ctx;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlagBits usage;
    VkImageAspectFlags aspectFlags;
    VkMemoryPropertyFlags memFlags;
    u32 arrayLayers;
    VkImageType type;
    VkImageViewType viewType;
    VkImageCreateFlags imageFlags;
} VulkanImageInfo;

typedef struct VulkanImage_s {
    VkImage image;
    VkImageView view;
    VkDeviceMemory mem;
    VkSampler sampler;

    VulkanImageInfo info;
} VulkanImage;

void VulkanImageCreate(VulkanImage* image, VulkanImageInfo info);
void VulkanImageDestroy(VulkanImage* image);

void VulkanImageSetPixels(VulkanImage* image, u8* pixels);
