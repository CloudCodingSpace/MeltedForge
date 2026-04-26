#ifdef __cpluscplus
extern "C" {
#endif

#include "mfskybox.h"

#include "mfpipeline.h"
#include "mfgpu_res.h"

#include "vk/backend.h"
#include "vk/image.h"
#include "vk/fb.h"
#include "vk/renderpass.h"
#include "vk/command_buffer.h"

struct MFSkybox_s {
    VulkanImage image;
    MFSkyboxConfig config;
    VulkanBackend* backend;
    bool init;
};

MFSkybox* mfSkyboxCreate(MFSkyboxConfig config, MFRenderer* renderer) {
    MF_PANIC_IF(config.hdrEnvironmentPath == mfnull, mfGetLogger(), "The hdr environment map path shouldn't be null!");
    MF_PANIC_IF(config.faceSize == 0, mfGetLogger(), "The face size of the skybox provided shouldn't be null!");

    MFSkybox* skybox = MF_ALLOCMEM(MFSkybox, sizeof(MFSkybox));

    skybox->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    skybox->config = config;
    skybox->config.hdrEnvironmentPath = mfStringDuplicate(config.hdrEnvironmentPath);

    VulkanImageInfo info = {
        .ctx = &skybox->backend->ctx,
        .arrayLayers = 6,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .generateMipmaps = false,
        .gpuResource = true,
        .width = config.faceSize,
        .height = config.faceSize,
        .imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .mipLevels = 1,
        .pixels = mfnull,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .type = VK_IMAGE_TYPE_2D,
        .viewType = VK_IMAGE_VIEW_TYPE_CUBE
    };
    VulkanImageCreate(&skybox->image, info);

    skybox->init = true;
    return skybox;
}

void mfSkyboxDestroy(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    VulkanImageDestroy(&skybox->image);

    MF_FREEMEM(skybox->config.hdrEnvironmentPath);
    MF_SETMEM(skybox, 0, sizeof(MFSkybox));
    MF_FREEMEM(skybox);
}

MFResourceDescription mfSkyboxGetDescription(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    return (MFResourceDescription){
        .binding = skybox->config.binding,
        .descriptorCount = 1,
        .descriptorType = MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = MF_SHADER_STAGE_FRAGMENT
    };
}

void mfSkyboxSetBinding(MFSkybox* skybox, u64 binding) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    skybox->config.binding = binding;
}

void* mfSkyboxGetBackend(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    return (void*)&skybox->image;
}

size_t mfSkyboxGetSizeInBytes(void) {
    return sizeof(MFSkybox);
}

#ifdef __cpluscplus
}
#endif