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
    bool gpuResource, generateMipmaps, storageImage;
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
    VkSampleCountFlagBits samples;
} VulkanImageInfo;

typedef struct VulkanImage_s {
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VmaAllocation allocation;
    
    VkFence fence;
    VkCommandBuffer cmdBuff;
    VkImageLayout layout;
    VkAccessFlagBits access;
    VkPipelineStageFlagBits stage;

    VulkanImageInfo info;
} VulkanImage;

void VulkanImageCreate(VulkanImage* image, VulkanImageInfo info);
void VulkanImageDestroy(VulkanImage* image);

void VulkanImageSetPixels(VulkanImage* image, u8* pixels);
u8* VulkanImageGetPixels(VulkanImage* image, u32 mipLevel, u32 faceIndex, u32* width, u32* height);
void VulkanImageGenerateMipmaps(VulkanImage* image, VkImageLayout oldLayout, VkAccessFlagBits srcAccess, VkPipelineStageFlagBits srcStage);
void VulkanImageTransitionLayout(VulkanImage* image, VkCommandBuffer cmdBuff, VkImageLayout dstLayout, VkAccessFlagBits dstAccess, VkPipelineStageFlagBits dstStage, VkImageSubresourceRange subResRange);

#ifdef __cplusplus
}
#endif