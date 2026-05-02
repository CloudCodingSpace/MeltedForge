#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "core/mfutils.h"

struct VulkanBackendCtx_s;

typedef struct VulkanImageInfo_s {
    u32 width, height, mipLevels;
    bool gpuResource, generateMipmaps;
    void* pixels;
    
    struct VulkanBackendCtx_s* ctx;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlagBits usage;
    VkImageAspectFlags aspectFlags;
    VmaMemoryUsage memFlags;
    u32 arrayLayers;
    VkImageType type;
    VkImageViewType viewType;
    VkImageCreateFlags imageFlags;
    VkSamplerAddressMode addressModes[3];
} VulkanImageInfo;

typedef struct VulkanImage_s {
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VmaAllocation allocation;

    VulkanImageInfo info;
} VulkanImage;

void VulkanImageCreate(VulkanImage* image, VulkanImageInfo info);
void VulkanImageDestroy(VulkanImage* image);

void VulkanImageSetPixels(VulkanImage* image, u8* pixels);

#ifdef __cplusplus
}
#endif