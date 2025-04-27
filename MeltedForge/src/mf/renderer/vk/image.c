#include "image.h"

#include "core/mfcore.h"

#include "ctx.h"
#include "common.h"

void VulkanImageCreate(VulkanImage* image, struct VulkanBackendCtx_s* ctx, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlagBits usage, VkImageAspectFlags aspectFlags, VkMemoryPropertyFlags memFlags) {
    image->width = width;    
    image->height = height;
    image->format = format;

    // Image
    {
        VkImageCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .arrayLayers = 1, // TODO: Make it configurable if required
            .extent = (VkExtent3D) {
                .depth = 1,
                .width = width,
                .height = height
            },
            .format = format,
            .imageType = VK_IMAGE_TYPE_2D, // TODO: Make it configurable if required
            .tiling = tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = usage,
            .samples = VK_SAMPLE_COUNT_1_BIT, // TODO: Make it configurable if required
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO: Make it configurable if required 
            .mipLevels = 1, // TODO: Make it configurable
        };

        VK_CHECK(vkCreateImage(ctx->device, &info, ctx->allocator, &image->image));
    }
    // Memory
    {
        VkMemoryRequirements req = {0};
        vkGetImageMemoryRequirements(ctx->device, image->image, &req);
    
        VkMemoryAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = req.size,
            .memoryTypeIndex = FindMemoryType(ctx->physicalDevice, req.memoryTypeBits, memFlags)
        };

        VK_CHECK(vkAllocateMemory(ctx->device, &info, ctx->allocator, &image->mem));

        VK_CHECK(vkBindImageMemory(ctx->device, image->image, image->mem, 0));
    }
    // Image View
    {
        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .components = (VkComponentMapping) {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .format = format,
            .image = image->image,
            .subresourceRange = (VkImageSubresourceRange) {
                .aspectMask = aspectFlags,
                .baseArrayLayer = 0,
                .baseMipLevel = 0,
                .layerCount = 1,
                .levelCount = 1
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D // TODO: Make it configurable it required
        };

        VK_CHECK(vkCreateImageView(ctx->device, &info, ctx->allocator, &image->view));
    }
}

void VulkanImageDestroy(VulkanImage* image, struct VulkanBackendCtx_s* ctx) {
    vkFreeMemory(ctx->device, image->mem, ctx->allocator);
    vkDestroyImageView(ctx->device, image->view, ctx->allocator);
    vkDestroyImage(ctx->device, image->image, ctx->allocator);

    MF_SETMEM(image, 0, sizeof(VulkanImage));
}