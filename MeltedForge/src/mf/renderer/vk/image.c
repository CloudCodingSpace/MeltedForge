#include "image.h"

#include "core/mfcore.h"

#include "ctx.h"
#include "common.h"
#include "buffer.h"
#include "cmd.h"

void VulkanImageCreate(VulkanImage* image, struct VulkanBackendCtx_s* ctx, u32 width, u32 height, b8 gpuResource, u8* pixels, VkFormat format, VkImageTiling tiling, VkImageUsageFlagBits usage, VkImageAspectFlags aspectFlags, VkMemoryPropertyFlags memFlags) {
    image->width = width;    
    image->height = height;
    image->format = format;
    image->gpuResource = gpuResource;
    image->pixels = pixels;

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
            .mipLevels = 1, // TODO: Make it configurable if required
        };

        if(gpuResource)
            info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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
            .viewType = VK_IMAGE_VIEW_TYPE_2D // TODO: Make it configurable if required
        };

        VK_CHECK(vkCreateImageView(ctx->device, &info, ctx->allocator, &image->view));
    }

    if(!image->gpuResource)
        return;

    // Samplers
    {
        VkPhysicalDeviceFeatures features = {0};
        vkGetPhysicalDeviceFeatures(ctx->physicalDevice, &features);
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(ctx->physicalDevice, &props);

        VkSamplerCreateInfo sinfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .anisotropyEnable = features.samplerAnisotropy,
            .maxAnisotropy = props.limits.maxSamplerAnisotropy,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .maxLod = 100,
            .minLod = -100,
            .mipLodBias = 0.0f,
            .unnormalizedCoordinates = VK_FALSE,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
        };

        VK_CHECK(vkCreateSampler(ctx->device, &sinfo, ctx->allocator, &image->sampler));
    }

    if(!pixels)
        return;

    VulkanImageSetPixels(image, ctx, pixels);
}

void VulkanImageDestroy(VulkanImage* image, struct VulkanBackendCtx_s* ctx) {
    vkFreeMemory(ctx->device, image->mem, ctx->allocator);
    vkDestroyImageView(ctx->device, image->view, ctx->allocator);
    vkDestroyImage(ctx->device, image->image, ctx->allocator);

    if(image->gpuResource)
        vkDestroySampler(ctx->device, image->sampler, ctx->allocator);

    MF_SETMEM(image, 0, sizeof(VulkanImage));
}

void VulkanImageSetPixels(VulkanImage* image, struct VulkanBackendCtx_s* ctx, u8* pixels) {
    image->pixels = pixels;

    VulkanBuffer staging = {};
    VulkanBufferAllocate(&staging, ctx, ctx->cmdPool, image->width * image->height * 4, pixels, VULKAN_BUFFER_TYPE_STAGING);

    // Upload to staging buffer
    void* mem;
    vkMapMemory(ctx->device, staging.mem, 0, image->width * image->height * 4, 0, &mem);
    memcpy(mem, pixels, image->width * image->height * 4);
    vkUnmapMemory(ctx->device, staging.mem);

    // Copy staging buffer to image and transitioning to the appropriate layout
    {
        VkCommandBuffer buff = VulkanCommandBufferAllocate(ctx, ctx->cmdPool, true);
        VulkanCommandBufferBegin(buff);

        {
            VkImageMemoryBarrier copy_barrier[1] = {};
			copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier[0].image = image->image;
			copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_barrier[0].subresourceRange.levelCount = 1;
			copy_barrier[0].subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, copy_barrier);
        }

        {
            VkBufferImageCopy region = {
                .imageOffset = (VkOffset3D){ 0, 0, 0 },
                .imageSubresource.mipLevel = 0,
                .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .imageSubresource.baseArrayLayer = 0,
                .imageSubresource.layerCount = 1,
                .imageExtent = (VkExtent3D){ (uint32_t)image->width, (uint32_t)image->height, 1 },
                .bufferImageHeight = 0,
                .bufferOffset = 0,
                .bufferRowLength = 0
            };
			vkCmdCopyBufferToImage(buff, staging.handle, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        {
            VkImageMemoryBarrier use_barrier[1] = {};
			use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier[0].image = image->image;
			use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			use_barrier[0].subresourceRange.levelCount = 1;
			use_barrier[0].subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, use_barrier);
        }

        VulkanCommandBufferEnd(buff);

        VkSubmitInfo sinfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &buff
        };

        VK_CHECK(vkQueueSubmit(ctx->qData.tQueue, 1, &sinfo, mfnull));
        vkDeviceWaitIdle(ctx->device);

        VulkanCommandBufferFree(ctx, buff, ctx->cmdPool);
    }

    VulkanBufferFree(&staging, ctx);
}