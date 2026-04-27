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
    MFGpuImage* image;
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

    MFGpuImageConfig info = {
        .binding = config.binding,
        .generateMipmaps = true,
        .width = config.faceSize,
        .height = config.faceSize,
        .isCubemap = true,
        .imageFormat = MF_FORMAT_R16G16B16A16_SFLOAT
    };
    skybox->image = mfGpuImageCreate(renderer, info);

    skybox->init = true;
    return skybox;
}

void mfSkyboxDestroy(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    mfGpuImageDestroy(skybox->image);
    
    MF_FREEMEM(skybox->config.hdrEnvironmentPath);
    MF_SETMEM(skybox, 0, sizeof(MFSkybox));
    MF_FREEMEM(skybox);
}

size_t mfSkyboxGetSizeInBytes(void) {
    return sizeof(MFSkybox);
}

void mfSkyboxRender(MFSkybox* skybox, MFMat4 projection, MFMat4 view, MFMat4 model) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");

    // TODO: Implement this function
}

MFGpuImage* mfSkyboxGetCubemapImage(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");

    return skybox->image;
}

#ifdef __cpluscplus
}
#endif