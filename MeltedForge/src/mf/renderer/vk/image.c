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
    image->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    image->access = 0;
    image->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    if(pinfo.generateMipmaps) {
        image->info.mipLevels = pinfo.mipLevels = floor(log2(MAX(pinfo.width, pinfo.height))) + 1;
    }

    VulkanBackendCtx* ctx = image->info.ctx;

    // Persistent layout objects
    {
        image->cmdBuff = VulkanCommandBufferAllocate(ctx, ctx->commandPool, true);
        VkFenceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };

        VK_CHECK(vkCreateFence(ctx->device, &info, ctx->allocator, &image->fence));
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
            .usage = pinfo.usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .samples = pinfo.samples,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .mipLevels = pinfo.mipLevels,
            .flags = pinfo.imageFlags,
            .queueFamilyIndexCount = ctx->uniqueQueueCount,
            .pQueueFamilyIndices = ctx->uniqueQueues
        };

        if(pinfo.storageImage) {
            info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if(ctx->uniqueQueueCount > 1) {
            info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        }

        VmaAllocationCreateInfo allocInfo = {
            .usage = pinfo.memFlags
        };

        VK_CHECK(vmaCreateImage(ctx->vmaAllocator, &info, &allocInfo, &image->image, &image->allocation, mfnull));
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

        VK_CHECK(vkCreateImageView(ctx->device, &info, ctx->allocator, &image->view));
    }

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Created an image of resolution: %dx%d", 
                    image->info.width, image->info.height);

    if(!pinfo.gpuResource)
        return;

    // Samplers
    {
        VkPhysicalDeviceFeatures features = {0};
        vkGetPhysicalDeviceFeatures(ctx->physicalDevice, &features);
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(ctx->physicalDevice, &props);

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

        VK_CHECK(vkCreateSampler(ctx->device, &sinfo, ctx->allocator, &image->sampler));
    }

    if(!pinfo.pixels)
        return;

    VulkanImageSetPixels(image, pinfo.pixels);
}

void VulkanImageDestroy(VulkanImage* image) {
    VulkanBackendCtx* ctx = image->info.ctx;

    vkDestroyFence(ctx->device, image->fence, ctx->allocator);
    VulkanCommandBufferFree(ctx, image->cmdBuff, ctx->commandPool);

    vkDestroyImageView(ctx->device, image->view, ctx->allocator);
    vmaDestroyImage(ctx->vmaAllocator, image->image, image->allocation);

    if(image->info.gpuResource) {
        vkDestroySampler(ctx->device, image->sampler, ctx->allocator);
    }

    MF_INFO(mfGetLogger(), "(From the vulkan backend) Destroyed an image of resolution: %dx%d",
                        image->info.width, image->info.height);

    MF_SETMEM(image, 0, sizeof(VulkanImage));
}

void VulkanImageSetPixels(VulkanImage* image, u8* pixels) {
    image->info.pixels = pixels;
    VulkanBackendCtx* ctx = image->info.ctx;
    VkDeviceSize size = image->info.arrayLayers * image->info.width * image->info.height * VulkanFormatBytesPerPixel(image->info.format);

    VulkanBufferInfo info = {
        .ctx = ctx,
        .pool = ctx->commandPool,
        .data = mfnull,
        .size = size,
        .type = VULKAN_BUFFER_TYPE_STAGING
    };
    VulkanBuffer staging = {};
    VulkanBufferAllocate(&staging, info);

    // Upload to staging buffer
    void* mem;
    VK_CHECK(vmaMapMemory(ctx->vmaAllocator, staging.allocation, &mem));
    memcpy(mem, pixels, size);
    vmaUnmapMemory(ctx->vmaAllocator, staging.allocation);

    // Copy staging buffer to image and transitioning to the appropriate layout
    {
        VulkanCommandBufferBegin(image->cmdBuff, true);

        VulkanImageTransitionLayout(image, image->cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange) {
            .aspectMask = image->info.aspectFlags,
            .levelCount = image->info.mipLevels,
            .layerCount = image->info.arrayLayers
        });

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
			vkCmdCopyBufferToImage(image->cmdBuff, staging.handle, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        if(!image->info.generateMipmaps && (image->info.mipLevels == 1)) {
            VulkanImageTransitionLayout(image, image->cmdBuff, image->info.storageImage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkImageSubresourceRange) {
                .aspectMask = image->info.aspectFlags,
                .levelCount = image->info.mipLevels,
                .layerCount = image->info.arrayLayers
            });
        }

        VulkanCommandBufferEnd(image->cmdBuff);

        VkSubmitInfo sinfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &image->cmdBuff
        };

        VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &sinfo, image->fence));
        VK_CHECK(vkWaitForFences(ctx->device, 1, &image->fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(ctx->device, 1, &image->fence));
        VK_CHECK(vkResetCommandBuffer(image->cmdBuff, 0));
    }

    VulkanBufferFree(&staging);

    if(image->info.generateMipmaps && (image->info.mipLevels > 1))
        VulkanImageGenerateMipmaps(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

u8* VulkanImageGetPixels(VulkanImage* image, u32 mipLevel, u32 faceIndex, u32* width, u32* height) {
    VulkanBackendCtx* ctx = image->info.ctx;
    
    u32 w  = MAX(1, image->info.width  >> mipLevel);
    u32 h = MAX(1, image->info.height >> mipLevel);
    *width = w;
    *height = h;
    u64 size = sizeof(u8) * VulkanFormatBytesPerPixel(image->info.format) * w * h;
    u8* buffer = MF_ALLOCMEM(u8, size);

    VulkanBuffer staging = {0};
    {
        VulkanBufferInfo buffInfo = {
            .frequentUpdates = true,
            .ctx = ctx,
            .data = mfnull,
            .pool = ctx->commandPool,
            .size = size,
            .type = VULKAN_BUFFER_TYPE_STAGING
        };
        VulkanBufferAllocate(&staging, buffInfo);
    }

    VulkanCommandBufferBegin(image->cmdBuff, true);

    VkAccessFlagBits access = image->access;
    VkPipelineStageFlagBits stage = image->stage;
    VkImageLayout layout = image->layout;

    VulkanImageTransitionLayout(image, image->cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange){
        .aspectMask = image->info.aspectFlags,
        .baseArrayLayer = faceIndex,
        .baseMipLevel = mipLevel,
        .layerCount = 1,
        .levelCount = 1
    });

    VkBufferImageCopy region = {
        .imageOffset = (VkOffset3D){ 0, 0, 0 },
        .imageSubresource.mipLevel = mipLevel,
        .imageSubresource.aspectMask = image->info.aspectFlags,
        .imageSubresource.baseArrayLayer = faceIndex,
        .imageSubresource.layerCount = 1,
        .imageExtent = (VkExtent3D){ (uint32_t)w, (uint32_t)h, 1 },
        .bufferImageHeight = 0,
        .bufferOffset = 0,
        .bufferRowLength = 0
    };
    vkCmdCopyImageToBuffer(image->cmdBuff, image->image, image->layout, staging.handle, 1, &region);

    VulkanImageTransitionLayout(image, image->cmdBuff, layout, access, stage, (VkImageSubresourceRange){
        .aspectMask = image->info.aspectFlags,
        .baseArrayLayer = faceIndex,
        .baseMipLevel = mipLevel,
        .layerCount = 1,
        .levelCount = 1
    });

    VulkanCommandBufferEnd(image->cmdBuff);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &image->cmdBuff
    };

    VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &submitInfo, image->fence));
    VK_CHECK(vkWaitForFences(ctx->device, 1, &image->fence, VK_TRUE, UINT64_MAX));

    memcpy(buffer, staging.mappedMem, size);

    VulkanBufferFree(&staging);
    VK_CHECK(vkResetCommandBuffer(image->cmdBuff, 0));

    return buffer;
}

void VulkanImageGenerateMipmaps(VulkanImage* image, VkImageLayout oldLayout, VkAccessFlagBits srcAccess, VkPipelineStageFlagBits srcStage) {
    VulkanBackendCtx* ctx = image->info.ctx;
    if(image->info.mipLevels == 1)
        return;
    
    image->layout = oldLayout;
    image->access = srcAccess;
    image->stage = srcStage;
    
    // Checking if image blit is supported or not!
    {
        VkFormatProperties props = {0};
        vkGetPhysicalDeviceFormatProperties(ctx->physicalDevice, image->info.format, &props);
        if(!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "(From the vulkan backend) VkFormat %s doesn't support blits, which is required for generating mipmaps!", string_VkFormat(image->info.format));
            return;
        }
    }

    VulkanCommandBufferBegin(image->cmdBuff, true);

    for(u32 layer = 0; layer < image->info.arrayLayers; layer++) {
        VulkanImageTransitionLayout(image, image->cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange) {
            .aspectMask = image->info.aspectFlags,
            .baseArrayLayer = layer,
            .layerCount = 1,
            .levelCount = 1,
            .baseMipLevel = 0
        });

        image->layout = oldLayout;
        image->access = srcAccess;
        image->stage = srcStage;
        
        VulkanImageTransitionLayout(image, image->cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange) {
            .aspectMask = image->info.aspectFlags,
            .baseArrayLayer = layer,
            .layerCount = 1,
            .levelCount = image->info.mipLevels - 1,
            .baseMipLevel = 1
        });

        u32 w = image->info.width;
        u32 h = image->info.height;

        for(i32 i = 1; i < image->info.mipLevels; i++) {
            VkImageBlit blit = {
                .srcOffsets[0] = {0, 0, 0},
                .srcOffsets[1] = {w, h, 1},
                .dstOffsets[0] = {0, 0, 0},
                .dstOffsets[1] = { (w > 1) ? w/2 : 1, (h > 1) ? h/2 : 1, 1},
                .srcSubresource = {
                    .aspectMask = image->info.aspectFlags,
                    .baseArrayLayer = layer,
                    .layerCount = 1,
                    .mipLevel = i - 1,
                },
                .dstSubresource = {
                    .aspectMask = image->info.aspectFlags,
                    .baseArrayLayer = layer,
                    .layerCount = 1,
                    .mipLevel = i,
                }
            };

            vkCmdBlitImage(image->cmdBuff, image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            image->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            VulkanImageTransitionLayout(image, image->cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange) {
                .aspectMask = image->info.aspectFlags,
                .baseArrayLayer = layer,
                .layerCount = 1,
                .levelCount = 1,
                .baseMipLevel = i
            });
            
            if(w > 1)
                w /= 2;
            if(h > 1)
                h /= 2;
        }
    }
    
    image->layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VulkanImageTransitionLayout(image, image->cmdBuff, image->info.storageImage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkImageSubresourceRange) {
        .aspectMask = image->info.aspectFlags,
        .baseArrayLayer = 0,
        .layerCount = image->info.arrayLayers,
        .baseMipLevel = 0,
        .levelCount = image->info.mipLevels
    });

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &image->cmdBuff
    };

    VulkanCommandBufferEnd(image->cmdBuff);

    VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &submit, image->fence));
    VK_CHECK(vkWaitForFences(ctx->device, 1, &image->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(ctx->device, 1, &image->fence));
    VK_CHECK(vkResetCommandBuffer(image->cmdBuff, 0));
}

void VulkanImageTransitionLayout(VulkanImage* image, VkCommandBuffer cmdBuff, VkImageLayout dstLayout, VkAccessFlagBits dstAccess, VkPipelineStageFlagBits dstStage, VkImageSubresourceRange subResRange) {
    VulkanBackendCtx* ctx = image->info.ctx;
    
    if(image->access == dstAccess && image->layout == dstLayout && image->stage == dstStage)
        return;

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = image->layout,
        .newLayout = dstLayout,
        .srcAccessMask = image->access,
        .dstAccessMask = dstAccess,
        .image = image->image,
        .subresourceRange = subResRange
    };
    vkCmdPipelineBarrier(cmdBuff, image->stage, dstStage,
                                0, 0, mfnull, 0, mfnull, 1, &barrier);

    image->layout = dstLayout;
    image->access = dstAccess;
    image->stage = dstStage;
}

#ifdef __cplusplus
}
#endif