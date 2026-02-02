#include "image.h"

#include "core/mfcore.h"

#include "ctx.h"
#include "common.h"
#include "buffer.h"
#include "cmd.h"

void VulkanImageCreate(VulkanImage* image, VulkanImageInfo pinfo) {
    image->info = pinfo;

    // Image
    {
        VkImageCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .arrayLayers = pinfo.arrayLayers,
            .extent = (VkExtent3D) {
                .depth = 1,
                .width = pinfo.width,
                .height = pinfo.height
            },
            .format = pinfo.format,
            .imageType = pinfo.type, 
            .tiling = pinfo.tiling,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = pinfo.usage,
            .samples = VK_SAMPLE_COUNT_1_BIT, // NOTE: Make it configurable if required
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // NOTE: Make it configurable if required 
            .mipLevels = 1, // NOTE: Make it configurable if required
            .flags = pinfo.imageFlags
        };

        if(pinfo.gpuResource)
            info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VK_CHECK(vkCreateImage(pinfo.ctx->device, &info, pinfo.ctx->allocator, &image->image));
    }
    // Memory
    {
        VkMemoryRequirements req = {0};
        vkGetImageMemoryRequirements(pinfo.ctx->device, image->image, &req);
    
        VkMemoryAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = req.size,
            .memoryTypeIndex = FindMemoryType(pinfo.ctx->physicalDevice, req.memoryTypeBits, pinfo.memFlags)
        };

        VK_CHECK(vkAllocateMemory(pinfo.ctx->device, &info, pinfo.ctx->allocator, &image->mem));

        VK_CHECK(vkBindImageMemory(pinfo.ctx->device, image->image, image->mem, 0));
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
            .format = pinfo.format,
            .image = image->image,
            .subresourceRange = (VkImageSubresourceRange) {
                .aspectMask = pinfo.aspectFlags,
                .baseArrayLayer = 0,
                .baseMipLevel = 0,
                .layerCount = 1,
                .levelCount = 1
            },
            .viewType = pinfo.viewType
        };

        VK_CHECK(vkCreateImageView(pinfo.ctx->device, &info, pinfo.ctx->allocator, &image->view));
    }

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Created an image of resolution: %dx%d", 
                    image->info.width, image->info.height);

    if(!pinfo.gpuResource)
        return;

    // Samplers
    {
        VkPhysicalDeviceFeatures features = {0};
        vkGetPhysicalDeviceFeatures(pinfo.ctx->physicalDevice, &features);
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(pinfo.ctx->physicalDevice, &props);

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

        VK_CHECK(vkCreateSampler(pinfo.ctx->device, &sinfo, pinfo.ctx->allocator, &image->sampler));
    }

    if(!pinfo.pixels)
        return;

    VulkanImageSetPixels(image, pinfo.pixels);
}

void VulkanImageDestroy(VulkanImage* image) {
    vkFreeMemory(image->info.ctx->device, image->mem, image->info.ctx->allocator);
    vkDestroyImageView(image->info.ctx->device, image->view, image->info.ctx->allocator);
    vkDestroyImage(image->info.ctx->device, image->image, image->info.ctx->allocator);

    if(image->info.gpuResource)
        vkDestroySampler(image->info.ctx->device, image->sampler, image->info.ctx->allocator);

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Destroyed an image of  resolution: %dx%d",
                        image->info.width, image->info.height);

    MF_SETMEM(image, 0, sizeof(VulkanImage));
}

void VulkanImageSetPixels(VulkanImage* image, u8* pixels) {
    image->info.pixels = pixels;

    VulkanBuffer staging = {};
    VulkanBufferAllocate(&staging, image->info.ctx, image->info.ctx->cmdPool, image->info.width * image->info.height * 4, pixels, VULKAN_BUFFER_TYPE_STAGING);

    // Upload to staging buffer
    void* mem;
    vkMapMemory(image->info.ctx->device, staging.mem, 0, image->info.width * image->info.height * 4, 0, &mem);
    memcpy(mem, pixels, image->info.width * image->info.height * 4);
    vkUnmapMemory(image->info.ctx->device, staging.mem);

    // Copy staging buffer to image and transitioning to the appropriate layout
    {
        VkCommandBuffer buff = VulkanCommandBufferAllocate(image->info.ctx, image->info.ctx->cmdPool, true);
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
                .imageExtent = (VkExtent3D){ (uint32_t)image->info.width, (uint32_t)image->info.height, 1 },
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

        VK_CHECK(vkQueueSubmit(image->info.ctx->qData.tQueue, 1, &sinfo, mfnull));
        vkDeviceWaitIdle(image->info.ctx->device);

        VulkanCommandBufferFree(image->info.ctx, buff, image->info.ctx->cmdPool);
    }

    VulkanBufferFree(&staging, image->info.ctx);
}
