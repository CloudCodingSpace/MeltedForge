#ifdef __cplusplus
extern "C" {
#endif

#include "mfgpuimage.h"

#include "vk/image.h"
#include "vk/ctx.h"
#include "vk/backend.h"

#include <cimgui.h>
#include <cimgui_impl.h>

struct MFGpuImage_s {
    VulkanImage image;
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    MFGpuImageConfig config;
    VkDescriptorSet igSets[FRAMES_IN_FLIGHT];
    bool init;
};

MFGpuImage* mfGpuImageCreate(MFRenderer* renderer, MFGpuImageConfig config) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFGpuImage* image = MF_ALLOCMEM(MFGpuImage, sizeof(MFGpuImage));
    
    image->config = config;
    image->backend = ((VulkanBackend*)mfRendererGetBackend(renderer));
    image->ctx = &image->backend->ctx;

    VulkanImageInfo info = {
        .ctx = image->ctx,
        .width = config.width,
        .height = config.height,
        .gpuResource = !config.isStorageImage,
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
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT
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
    
    if(config.forImguiTexture && image->backend->config.enableUI) {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            image->igSets[i] = ImGui_ImplVulkan_AddTexture(image->image.sampler, image->image.view, config.isStorageImage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
    
    VulkanImageCreate(&image->image, info);

    image->init = true;
    return image;
}

void mfGpuImageDestroy(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
 
    
    if(image->config.forImguiTexture && image->backend->config.enableUI) {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            ImGui_ImplVulkan_RemoveTexture(image->igSets[i]);
        }
    }

    VulkanImageDestroy(&image->image);

    MF_SETMEM(image, 0, sizeof(MFGpuImage));
    MF_FREEMEM(image);
}

ImTextureID mfGpuImageGetImGuiTextureID(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");

    return (ImTextureID)image->igSets[image->backend->frameIndex];
}

u8* mfGpuImageGetPixels(MFGpuImage* image, u32* width, u32* height, u32 mipLevel, u32 faceIndex) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    MF_PANIC_IF(width == mfnull, mfGetLogger(), "The width pointer provided shouldn't be null!");
    MF_PANIC_IF(height == mfnull, mfGetLogger(), "The height pointer provided shouldn't be null!");

    VulkanImage* backend = &image->image;
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

    VulkanImageSetPixels(&image->image, image->config.pixels);
}

void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(!image->init, mfGetLogger(), "The gpu image isn't initialised!");
    
    image->config.width = width;
    image->config.height = height;

    VulkanImageDestroy(&image->image);

    VulkanImageInfo info = {
        .ctx = image->ctx,
        .width = image->config.width,
        .height = image->config.height,
        .gpuResource = true,
        .pixels = image->config.pixels,
        .format = (VkFormat)(u32)(image->config.imageFormat),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
        .generateMipmaps = image->config.generateMipmaps,
        .samples = VK_SAMPLE_COUNT_1_BIT
    };
    
    VulkanImageCreate(&image->image, info);
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
    
    return &image->image;
}

MFGpuImage* mfCreateErrorGpuImage(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFGpuImage* tex = MF_ALLOCMEM(MFGpuImage, mfGpuImageGetSizeInBytes());
    
    MFGpuImageConfig config = {
        .width = mfGetErrorImageWidth(),
        .height = mfGetErrorImageHeight(),
        .pixels = mfGetErrorImagePixels(),
        .imageFormat = MF_FORMAT_R8G8B8A8_SRGB
    };

    return mfGpuImageCreate(renderer, config);
}

#ifdef __cplusplus
}
#endif