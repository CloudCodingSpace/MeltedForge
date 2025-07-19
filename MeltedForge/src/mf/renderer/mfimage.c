#include "mfimage.h"

#include "vk/image.h"
#include "vk/ctx.h"
#include "vk/backend.h"

struct MFGpuImage_s {
    VulkanImage image;
    VulkanBackendCtx* ctx;
    MFGpuImageConfig config;
};

void mfGpuImageCreate(MFGpuImage* image, MFRenderer* renderer, MFGpuImageConfig config) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    image->config = config;
    image->ctx = &((VulkanBackend*)mfRendererGetBackend(renderer))->ctx;
    
    VulkanImageCreate(&image->image, image->ctx, config.width, config.height, true, config.pixels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void mfGpuImageDestroy(MFGpuImage* image) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
 
    VulkanImageDestroy(&image->image, image->ctx);

    MF_SETMEM(image, 0, sizeof(MFGpuImage));
}

void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    MF_ASSERT(pixels == mfnull, mfGetLogger(), "The pixels provided shouldn't be null!");
    
    memcpy(image->config.pixels, pixels, sizeof(u8) * image->config.width * image->config.height * 4);

    VulkanImageSetPixels(&image->image, image->ctx, image->config.pixels);
}

void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    
    image->config.width = width;
    image->config.height = height;

    VulkanImageDestroy(&image->image, image->ctx);

    VulkanImageCreate(&image->image, image->ctx, image->config.width, image->config.height, true, image->config.pixels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

const MFGpuImageConfig* mfGetGpuImageConfig(MFGpuImage* image) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    
    return &image->config;
}

size_t mfGetGpuImageSizeInBytes() {
    return sizeof(MFGpuImage);
}

MFResourceDesc mfGetGpuImageDescription(MFGpuImage* image) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");

    return (MFResourceDesc) {
        .binding = image->config.binding,
        .descriptorCount = 1, // TODO: Make it configurable if required
        .descriptorType = MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = MF_SHADER_STAGE_FRAGMENT // TODO: Make it configurable if required
    };
}

struct VulkanImage_s mfGetGpuImageBackend(MFGpuImage* image) {
    MF_ASSERT(image == mfnull, mfGetLogger(), "The image handle provided shouldn't be null!");
    
    return image->image;
}