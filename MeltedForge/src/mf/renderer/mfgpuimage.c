#include "mfgpuimage.h"

#include "vk/image.h"
#include "vk/ctx.h"
#include "vk/backend.h"

struct MFGpuImage_s {
    VulkanImage image;
    VulkanBackendCtx* ctx;
    MFGpuImageConfig config;
};

void mfGpuImageCreate(MFGpuImage* image, MFRenderer* renderer, MFGpuImageConfig config) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    image->config = config;
    image->ctx = &((VulkanBackend*)mfRendererGetBackend(renderer))->ctx;

    VulkanImageInfo info = {
        .ctx = image->ctx,
        .width = config.width,
        .height = config.height,
        .gpuResource = true,
        .pixels = config.pixels,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .arrayLayers = 1,
        .type = VK_IMAGE_TYPE_2D,
        .viewType = VK_IMAGE_VIEW_TYPE_2D
    };
    
    VulkanImageCreate(&image->image, info);
}

void mfGpuImageDestroy(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
 
    VulkanImageDestroy(&image->image);

    MF_SETMEM(image, 0, sizeof(MFGpuImage));
}

void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(pixels == mfnull, mfGetLogger(), "The pixels provided shouldn't be null!");
    
    memcpy(image->config.pixels, pixels, sizeof(u8) * image->config.width * image->config.height * 4);

    VulkanImageSetPixels(&image->image, image->config.pixels);
}

void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    
    image->config.width = width;
    image->config.height = height;

    VulkanImageDestroy(&image->image);

    VulkanImageInfo info = {
        .ctx = image->ctx,
        .width = image->config.width,
        .height = image->config.height,
        .gpuResource = true,
        .pixels = image->config.pixels,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    
    VulkanImageCreate(&image->image, info);
}

const MFGpuImageConfig* mfGpuImageGetConfig(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    
    return &image->config;
}

size_t mfGpuImageGetSizeInBytes(void) {
    return sizeof(MFGpuImage);
}

void mfGpuImageSetBinding(MFGpuImage* image, u32 binding) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_PANIC_IF(binding < 0 || binding > 100, mfGetLogger(), "The binding slot provided should be valid!");

    image->config.binding = binding;
}

MFResourceDesc mGpuImageGetDescription(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");

    return (MFResourceDesc) {
        .binding = image->config.binding,
        .descriptorCount = 1, // NOTE: Make it configurable if required
        .descriptorType = MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = MF_SHADER_STAGE_FRAGMENT // NOTE: Make it configurable if required
    };
}

struct VulkanImage_s mfGpuImageGetBackend(MFGpuImage* image) {
    MF_PANIC_IF(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    
    return image->image;
}

MFGpuImage* mfCreateErrorGpuImage(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    u8 invColor[4 * 16 * 16] = {0};
    u32 cellSize = 4;
    for(u32 h = 0; h < 16; h++) {
        for(u32 w = 0; w < 16; w++) {
            u32 idx = (h * 16 + w) * 4;
            u32 x = w / cellSize;
            u32 y = h / cellSize;

            if(((x + y) % 2) == 0) {
                invColor[idx + 0] = 0xff;
                invColor[idx + 1] = 0x00;
                invColor[idx + 2] = 0xff;
                invColor[idx + 3] = 0xff;
            } else {
                invColor[idx + 0] = 0x00;
                invColor[idx + 1] = 0x00;
                invColor[idx + 2] = 0x00;
                invColor[idx + 3] = 0xff;
            }
        }
    }

    MFGpuImage* tex = MF_ALLOCMEM(MFGpuImage, mfGpuImageGetSizeInBytes());
    
    MFGpuImageConfig config = {
        .width = 16,
        .height = 16,
        .pixels = invColor,
        .binding = MF_INFINITY
    };

    mfGpuImageCreate(tex, renderer, config);

    return tex;
}