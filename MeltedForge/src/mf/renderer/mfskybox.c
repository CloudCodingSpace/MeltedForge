#ifdef __cpluscplus
extern "C" {
#endif

#include "mfskybox.h"

#include "objects/mfmesh.h"

#include "mfpipeline.h"
#include "mfgpu_res.h"

#include "vk/backend.h"
#include "vk/image.h"
#include "vk/fb.h"
#include "vk/renderpass.h"
#include "vk/command_buffer.h"

#include <stb/stb_image.h>

struct MFSkybox_s {
    MFMesh mesh;
    MFPipeline* pipeline;
    MFResourceSet* set;
    MFResourceSetLayout* layout;

    MFGpuImage* image;
    MFSkyboxConfig config;
    VulkanBackend* backend;
    bool init;
};

static void convertEnvMapToSkybox(MFSkybox* skybox, MFSkyboxConfig config);

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
        .imageFormat = MF_FORMAT_R16G16B16A16_SFLOAT,
        .binding = 0
    };
    skybox->image = mfGpuImageCreate(renderer, info);

    // Skybox
    {
        f32 vertices[] = {
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f
        };

        u32 indices[] = {
            0, 2, 1,
            0, 3, 2,

            4, 3, 0,
            4, 7, 3,

            1, 2, 6,
            1, 6, 5,

            5, 6, 7,
            5, 7, 4,

            3, 7, 6,
            3, 6, 2,

            4, 0, 1,
            4, 1, 5
        };

        mfMeshCreate(&skybox->mesh, renderer, sizeof(f32) * MF_ARRAYLEN(vertices, f32), vertices, MF_ARRAYLEN(indices, u32), indices);
    }
    // Resources
    {
        MFResourceDescription description = mfGpuImageGetDescription(skybox->image);
        skybox->layout = mfResourceSetLayoutCreate(1, &description, 1, renderer);

        skybox->set = mfResourceSetCreate(skybox->layout, renderer);

        {
            MFArray array = mfArrayCreate(mfGetLogger(), 1, sizeof(MFGpuImage*));
            mfArrayAddElement(array, MFGpuImage*, mfGetLogger(), skybox->image);
            mfResourceSetUpdate(skybox->set, &array, mfnull);
            mfArrayDestroy(&array, mfGetLogger());
        }
    }
    // Pipeline
    {
        MFVertexInputBindingDescription binding = {
            .binding = 0,
            .rate = MF_VERTEX_INPUT_RATE_VERTEX,
            .stride = sizeof(f32) * 3
        };

        MFVertexInputAttributeDescription attribute = {
            .binding = 0,
            .format = MF_FORMAT_R32G32B32_SFLOAT,
            .location = 0,
            .offset = 0
        };

        MFPushConstantRange range = {
            .offset = 0,
            .size = sizeof(MFMat4) * 2,
            .stage = MF_SHADER_STAGE_VERTEX
        };

        MFPipelineConfig info = {
            .hasDepth = true,
            .extent = (MFVec2){100.0f, 100.0f}, // NOTE: Leaving at 100,100 since pipeline supports dynamic rendering, that time this exten won't matter.
            .vertPath = "mfassets/shaders/mfskybox.vert.spv",
            .fragPath = "mfassets/shaders/mfskybox.frag.spv",
            .attributesCount = 1,
            .attributes = &attribute,
            .bindingsCount = 1,
            .bindings = &binding,
            .pushConstRangeCount = 1,
            .pushConstRanges = &range,
            .depthCompareOp = MF_COMPARE_OP_LESS_OR_EQUAL,
            .resourceLayoutCount = 1,
            .resourceLayouts = &skybox->layout
        };

        skybox->pipeline = mfPipelineCreate(renderer, &info);
    }

    convertEnvMapToSkybox(skybox, config);

    skybox->init = true;
    return skybox;
}

void mfSkyboxDestroy(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    mfMeshDestroy(&skybox->mesh);
    mfPipelineDestroy(skybox->pipeline);
    mfResourceSetDestroy(skybox->set);
    mfResourceSetLayoutDestroy(skybox->layout);
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

static void convertEnvMapToSkybox(MFSkybox* skybox, MFSkyboxConfig config) {

}

#ifdef __cpluscplus
}
#endif