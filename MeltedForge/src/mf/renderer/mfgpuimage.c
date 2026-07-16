#ifdef __cplusplus
extern "C" {
#endif

#include "mfgpuimage.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/image.h"
#include "vk/command_buffer.h"

#include <cimgui.h>
#include <cimgui_impl.h>

#include <math.h>

struct MFGpuImage_s {
    VulkanImage image[FRAMES_IN_FLIGHT];
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    MFGpuImageConfig config;
    VkDescriptorSet igSets[FRAMES_IN_FLIGHT][FRAMES_IN_FLIGHT];
    bool init;
};

MFGpuImage* mfGpuImageCreate(MFRenderer* renderer, MFGpuImageConfig config) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFGpuImage* image = MF_ALLOCMEM(MFGpuImage, sizeof(MFGpuImage));
    
    image->config = config;
    image->config.pixels = mfnull;
    image->backend = ((VulkanBackend*)mfRendererGetBackend(renderer));
    image->ctx = &image->backend->ctx;

    VulkanImageInfo info = {
        .ctx = image->ctx,
        .width = config.width,
        .height = config.height,
        .gpuResource = true,
        .pixels = config.pixels,
        .format = (VkFormat)(u32)(config.imageFormat),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
        .arrayLayers = 1,
        .type = VK_IMAGE_TYPE_2D,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .generateMipmaps = config.generateMipmaps,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .addressModes = {
            (VkSamplerAddressMode)(u32)config.addressMode,
            (VkSamplerAddressMode)(u32)config.addressMode,
            (VkSamplerAddressMode)(u32)config.addressMode
        },
        .storageImage = config.isStorageImage
    };

    if(config.isCubemap) {
        info.arrayLayers = 6;
        info.imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        info.addressModes[0] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModes[1] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModes[2] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    if(config.isColorAttachment)
        info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    for(u32 i = 0; i < (config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanImageCreate(&image->image[i], info);
   
    if(config.forImguiTexture && image->backend->config.enableUI) {
        for(u32 j = 0; j < (config.frameSynced ? FRAMES_IN_FLIGHT : 1); j++) {
            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
                image->igSets[j][i] = ImGui_ImplVulkan_AddTexture(image->image[j].sampler, image->image[j].view, config.isStorageImage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }
    }

    image->init = true;
    return image;
}

void mfGpuImageDestroy(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    
    if(image->config.forImguiTexture && image->backend->config.enableUI) {
        for(u32 j = 0; j < (image->config.frameSynced ? FRAMES_IN_FLIGHT : 1); j++) {
            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
                ImGui_ImplVulkan_RemoveTexture(image->igSets[j][i]);
            }
        }
    }

    for(u32 i = 0; i < (image->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanImageDestroy(&image->image[i]);

    MF_SETMEM(image, 0, sizeof(MFGpuImage));
    MF_FREEMEM(image);
}

ImTextureID mfGpuImageGetImGuiTextureID(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");

    u32 imgIdx = image->config.frameSynced ? image->backend->frameIndex : 0;
    return (ImTextureID)image->igSets[imgIdx][image->backend->frameIndex];
}

u8* mfGpuImageGetPixels(MFGpuImage* image, u32* width, u32* height, u32 mipLevel, u32 faceIndex) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    MF_PANIC_IF(width == mfnull, mfGetLogger(), "The width pointer provided shouldn't be null!");
    MF_PANIC_IF(height == mfnull, mfGetLogger(), "The height pointer provided shouldn't be null!");

    u32 imgIdx = image->config.frameSynced ? image->backend->frameIndex : 0;
    VulkanImage* backend = &image->image[imgIdx];
    if(backend->info.arrayLayers <= faceIndex) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The faceIndex isn't valid for getting the image's pixels! Returning the max. possible faceIndex available!");
        faceIndex = backend->info.arrayLayers - 1;
    }
    if(backend->info.mipLevels <= mipLevel) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The mipLevel isn't valid for getting the image's pixels! Returning the max. possible mip level available!");
        mipLevel = backend->info.mipLevels - 1;
    }

    return VulkanImageGetPixels(backend, mipLevel, faceIndex, width, height);
}

void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    MF_PANIC_IF(pixels == mfnull, mfGetLogger(), "The pixels provided shouldn't be null!");

    for(u32 i = 0; i < (image->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanImageSetPixels(&image->image[i], pixels);
}

void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height, u8* pixels) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    MF_PANIC_IF(pixels == mfnull, mfGetLogger(), "The pixels provided shouldn't be null!");
    
    image->config.width = width;
    image->config.height = height;

    for(u32 i = 0; i < (image->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanImageDestroy(&image->image[i]);

    VulkanImageInfo info = {
        .ctx = image->ctx,
        .width = image->config.width,
        .height = image->config.height,
        .gpuResource = true,
        .pixels = pixels,
        .format = (VkFormat)(u32)(image->config.imageFormat),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
        .generateMipmaps = image->config.generateMipmaps,
        .samples = VK_SAMPLE_COUNT_1_BIT
    };
    
    for(u32 i = 0; i < (image->config.frameSynced ? FRAMES_IN_FLIGHT : 1); i++)
        VulkanImageCreate(&image->image[i], info);
}

const MFGpuImageConfig* mfGpuImageGetConfig(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    
    return &image->config;
}

size_t mfGpuImageGetSizeInBytes(void) {
    return sizeof(MFGpuImage);
}

bool mfGpuImageIsValid(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");

    return image->init;
}

MFResourceDescription mfGpuImageGetDescription(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");

    return (MFResourceDescription) {
        .descriptorCount = 1, // NOTE: Make it configurable if required
        .descriptorType = image->config.isStorageImage ? MF_RES_DESCRIPTION_TYPE_STORAGE_IMAGE : MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = MF_SHADER_STAGE_FRAGMENT | MF_SHADER_STAGE_COMPUTE // NOTE: Make it configurable if required
    };
}

void* mfGpuImageGetBackend(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    
    return image->image;
}

MFGpuImage* mfCreateErrorGpuImage(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFGpuImage* tex = MF_ALLOCMEM(MFGpuImage, mfGpuImageGetSizeInBytes());
    
    MFGpuImageConfig config = {
        .width = mfGetErrorImageWidth(),
        .height = mfGetErrorImageHeight(),
        .pixels = mfGetErrorImagePixels(),
        .imageFormat = MF_FORMAT_R8G8B8A8_SRGB,
        .addressMode = MF_SAMPLER_ADDRESS_MODE_REPEAT
    };

    return mfGpuImageCreate(renderer, config);
}

void mfGpuImageCopy(MFGpuImage* src, MFGpuImage* dst) {
    MF_PANIC_IF(src == mfnull, mfGetLogger(), "The provided src MFGpuImage provided for copy shouldn't be null!");
    MF_PANIC_IF(!src->init, mfGetLogger(), "The provided src MFGpuImage provided for copy should be initialised!");
    MF_PANIC_IF(dst == mfnull, mfGetLogger(), "The provided dst MFGpuImage provided for copy shouldn't be null!");
    MF_PANIC_IF(!dst->init, mfGetLogger(), "The provided dst MFGpuImage provided for copy should be initialised!");

    bool sameParameters = (src->config.width == dst->config.width) && (src->config.height == dst->config.height) && (src->config.imageFormat == dst->config.imageFormat);
    MF_PANIC_IF(!sameParameters, mfGetLogger(), "The src and dst MFGpuImage for copy must be identical like same dimensions and image format!");

    VulkanBackend* backend = src->backend;
    VulkanBackendCtx* ctx = &backend->ctx;
    VulkanImage* srcImg = &src->image[src->config.frameSynced ? backend->frameIndex : 0];
    VulkanImage* dstImg = &dst->image[dst->config.frameSynced ? backend->frameIndex : 0];
    u32 minArrayLayers = srcImg->info.arrayLayers < dstImg->info.arrayLayers ? srcImg->info.arrayLayers : dstImg->info.arrayLayers;
    u32 minMipLevels = srcImg->info.mipLevels < dstImg->info.mipLevels ? srcImg->info.mipLevels : dstImg->info.mipLevels;

    VkImageLayout srcLayout = srcImg->layout;
    VkImageLayout dstLayout = dstImg->layout;
    VkAccessFlagBits srcAccess = srcImg->access;
    VkAccessFlagBits dstAccess = dstImg->access;
    VkPipelineStageFlagBits srcStage = srcImg->stage;
    VkPipelineStageFlagBits dstStage = dstImg->stage;

    VkCommandBuffer cmdBuff = srcImg->cmdBuff;
    VkFence fence = srcImg->fence;

    {
        VulkanCommandBufferBegin(cmdBuff, true);

        VulkanImageTransitionLayout(srcImg, cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Make it configurable if required
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .layerCount = minArrayLayers, // TODO: Make it configurable if required
            .levelCount = minMipLevels // TODO: Make it configurable if required
        });

        VulkanImageTransitionLayout(dstImg, cmdBuff, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Make it configurable if required
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .layerCount = minArrayLayers, // TODO: Make it configurable if required
            .levelCount = minMipLevels // TODO: Make it configurable if required
        });
        
        VkImageCopy region = {
            .srcOffset = {0, 0, 0},
            .dstOffset = {0, 0, 0},
            .extent = { srcImg->info.width, srcImg->info.height, 1 },
            .srcSubresource = {
                .mipLevel = 0, // TODO: Make it configurable if required
                .layerCount = minArrayLayers, // TODO: Make it configurable if required
                .baseArrayLayer = 0, // TODO: Make it configurable if required
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT // TODO: Make it configurable if required
            },
            .dstSubresource = {
                .mipLevel = 0, // TODO: Make it configurable if required
                .layerCount = minArrayLayers, // TODO: Make it configurable if required
                .baseArrayLayer = 0, // TODO: Make it configurable if required
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT // TODO: Make it configurable if required
            }
        };
        vkCmdCopyImage(cmdBuff, srcImg->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImg->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VulkanImageTransitionLayout(srcImg, cmdBuff, srcLayout, srcAccess, srcStage, (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Make it configurable if required
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .layerCount = minArrayLayers, // TODO: Make it configurable if required
            .levelCount = minMipLevels // TODO: Make it configurable if required
        });

        VulkanImageTransitionLayout(dstImg, cmdBuff, dstLayout, dstAccess, dstStage, (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Make it configurable if required
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .layerCount = minArrayLayers, // TODO: Make it configurable if required
            .levelCount = minMipLevels // TODO: Make it configurable if required
        });
        
        VulkanCommandBufferEnd(cmdBuff);
    }

    VkSubmitInfo sInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuff
    };
    VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &sInfo, fence));
    VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(ctx->device, 1, &fence));

    if(dstImg->info.generateMipmaps)
        VulkanImageGenerateMipmaps(dstImg, dstLayout, dstAccess, dstStage);
}

#ifdef __cplusplus
}
#endif