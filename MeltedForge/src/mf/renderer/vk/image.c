#ifdef __cplusplus
extern "C" {
#endif

#include "image.h"

#include "core/mfcore.h"

#include "ctx.h"
#include "common.h"
#include "buffer.h"
#include "command_buffer.h"

#include <vulkan/vk_enum_string_helper.h>

#include <math.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

void VulkanImageCreate(VulkanImage* image, VulkanImageInfo pinfo) {
    image->info = pinfo;
    image->info.mipLevels = pinfo.mipLevels = 1;

    if(pinfo.generateMipmaps) {
        image->info.mipLevels = pinfo.mipLevels = floor(log2(MAX(pinfo.width, pinfo.height))) + 1;
    }

    // Image & Memory
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
            .mipLevels = pinfo.mipLevels,
            .flags = pinfo.imageFlags
        };

        if(pinfo.generateMipmaps) {
            info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        if(pinfo.gpuResource)
            info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo = {
            .usage = pinfo.memFlags
        };

        VK_CHECK(vmaCreateImage(image->info.ctx->vmaAllocator, &info, &allocInfo, &image->image, &image->allocation, mfnull));
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
                .layerCount = pinfo.arrayLayers,
                .levelCount = pinfo.mipLevels
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
            .addressModeU = pinfo.addressModes[0],
            .addressModeV = pinfo.addressModes[1],
            .addressModeW = pinfo.addressModes[2],
            .anisotropyEnable = features.samplerAnisotropy,
            .maxAnisotropy = props.limits.maxSamplerAnisotropy,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .maxLod = 1.0f,
            .minLod = 0.0f,
            .mipLodBias = 0.0f,
            .unnormalizedCoordinates = VK_FALSE,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
        };

        if(pinfo.generateMipmaps) {
            sinfo.minLod = 0.0f;
            sinfo.maxLod = VK_LOD_CLAMP_NONE;
        }

        VK_CHECK(vkCreateSampler(pinfo.ctx->device, &sinfo, pinfo.ctx->allocator, &image->sampler));
    }

    if(!pinfo.pixels)
        return;

    VulkanImageSetPixels(image, pinfo.pixels);
}

void VulkanImageDestroy(VulkanImage* image) {
    VulkanBackendCtx* ctx = image->info.ctx;
    vkDestroyImageView(ctx->device, image->view, ctx->allocator);
    vmaDestroyImage(ctx->vmaAllocator, image->image, image->allocation);

    if(image->info.gpuResource)
        vkDestroySampler(ctx->device, image->sampler, ctx->allocator);

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Destroyed an image of resolution: %dx%d",
                        image->info.width, image->info.height);

    MF_SETMEM(image, 0, sizeof(VulkanImage));
}

void VulkanImageSetPixels(VulkanImage* image, u8* pixels) {
    image->info.pixels = pixels;
    VulkanBackendCtx* ctx = image->info.ctx;
    VkDeviceSize size = image->info.arrayLayers * image->info.width * image->info.height * VulkanFormatBytesPerPixel(image->info.format);

    VulkanBuffer staging = {};
    VulkanBufferAllocate(&staging, ctx, ctx->commandPool, size, mfnull, VULKAN_BUFFER_TYPE_STAGING);

    // Upload to staging buffer
    void* mem;
    VK_CHECK(vmaMapMemory(ctx->vmaAllocator, staging.allocation, &mem));
    memcpy(mem, pixels, size);
    vmaUnmapMemory(ctx->vmaAllocator, staging.allocation);

    // Copy staging buffer to image and transitioning to the appropriate layout
    {
        VkCommandBuffer buff = VulkanCommandBufferAllocate(ctx, ctx->commandPool, true);
        VulkanCommandBufferBegin(buff);
        
        VkFence fence = VK_NULL_HANDLE;
        {
            VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
            };
            VK_CHECK(vkCreateFence(ctx->device, &fenceInfo, ctx->allocator, &fence));
        }

        {
            VkImageMemoryBarrier copy_barrier[1] = {};
			copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier[0].image = image->image;
			copy_barrier[0].subresourceRange.aspectMask = image->info.aspectFlags;
			copy_barrier[0].subresourceRange.levelCount = image->info.mipLevels;
			copy_barrier[0].subresourceRange.layerCount = image->info.arrayLayers;
			vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, copy_barrier);
        }

        {
            VkBufferImageCopy region = {
                .imageOffset = (VkOffset3D){ 0, 0, 0 },
                .imageSubresource.mipLevel = 0,
                .imageSubresource.aspectMask = image->info.aspectFlags,
                .imageSubresource.baseArrayLayer = 0,
                .imageSubresource.layerCount = image->info.arrayLayers,
                .imageExtent = (VkExtent3D){ (uint32_t)image->info.width, (uint32_t)image->info.height, 1 },
                .bufferImageHeight = 0,
                .bufferOffset = 0,
                .bufferRowLength = 0
            };
			vkCmdCopyBufferToImage(buff, staging.handle, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        if(!image->info.generateMipmaps && (image->info.mipLevels == 1)) {
            VkImageMemoryBarrier use_barrier[1] = {};
			use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier[0].image = image->image;
			use_barrier[0].subresourceRange.aspectMask = image->info.aspectFlags;
			use_barrier[0].subresourceRange.levelCount = image->info.mipLevels;
			use_barrier[0].subresourceRange.layerCount = image->info.arrayLayers;
			vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, use_barrier);
        }

        VulkanCommandBufferEnd(buff);

        VkSubmitInfo sinfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &buff
        };

        VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &sinfo, fence));
        VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));

        VulkanCommandBufferFree(ctx, buff, ctx->commandPool);
        vkDestroyFence(ctx->device, fence, ctx->allocator);
    }

    VulkanBufferFree(&staging, ctx);

    if(image->info.generateMipmaps && (image->info.mipLevels > 1)) {
        // Checking if image blit is supported or not!
        {
            VkFormatProperties props = {0};
            vkGetPhysicalDeviceFormatProperties(ctx->physicalDevice, image->info.format, &props);
            if(!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "(From the vulkan backend) VkFormat %s doesn't support blits, which is required for generating mipmaps!", string_VkFormat(image->info.format));
                return;
            }
        }

        VkCommandBuffer cmd = VulkanCommandBufferAllocate(ctx, ctx->commandPool, true);
        VkFence fence;
        {
            VkFenceCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
            };

            VK_CHECK(vkCreateFence(ctx->device, &info, ctx->allocator, &fence));
        }

        VulkanCommandBufferBegin(cmd);

        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .subresourceRange = {
                .aspectMask = image->info.aspectFlags,
                .baseArrayLayer = 0,
                .layerCount = image->info.arrayLayers,
                .levelCount = 1,
                .baseMipLevel = 0
            },
            .image = image->image
        };

        u32 w = image->info.width;
        u32 h = image->info.height;

        for(i32 i = 1; i < image->info.mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, mfnull, 0, mfnull, 1, &barrier);
        
            VkImageBlit blit = {
                .srcOffsets[0] = {0, 0, 0},
                .srcOffsets[1] = {w, h, 1},
                .dstOffsets[0] = {0, 0, 0},
                .dstOffsets[1] = { (w > 1) ? w/2 : 1, (h > 1) ? h/2 : 1, 1},
                .srcSubresource = {
                    .aspectMask = image->info.aspectFlags,
                    .baseArrayLayer = 0,
                    .layerCount = image->info.arrayLayers,
                    .mipLevel = i - 1,
                },
                .dstSubresource = {
                    .aspectMask = image->info.aspectFlags,
                    .baseArrayLayer = 0,
                    .layerCount = image->info.arrayLayers,
                    .mipLevel = i,
                }
            };

            vkCmdBlitImage(cmd, image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, mfnull, 0, mfnull, 1, &barrier);
            
            if(w > 1)
                w /= 2;
            if(h > 1)
                h /= 2;
        }

        barrier.subresourceRange.baseMipLevel = image->info.mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, mfnull, 0, mfnull, 1, &barrier);
        
        VkSubmitInfo submit = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd
        };

        VulkanCommandBufferEnd(cmd);

        VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &submit, fence));
        VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(ctx->device, fence, ctx->allocator);
        VulkanCommandBufferFree(ctx, cmd, ctx->commandPool);
    }
}

#ifdef __cplusplus
}
#endif