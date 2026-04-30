#ifdef __cpluscplus
extern "C" {
#endif

#include "mfskybox.h"

#include "objects/mfmesh.h"

#include "mfpipeline.h"
#include "mfgpu_res.h"

#include "vk/backend.h"
#include "vk/skybox.h"

MFSkybox* mfSkyboxCreate(MFSkyboxConfig config, MFRenderer* renderer) {
    MF_PANIC_IF(config.environmentPath == mfnull, mfGetLogger(), "The hdr environment map path shouldn't be null!");
    MF_PANIC_IF(config.faceSize == 0, mfGetLogger(), "The face size of the skybox provided shouldn't be null!");

    MFSkybox* skybox = MF_ALLOCMEM(MFSkybox, sizeof(MFSkybox));

    skybox->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    skybox->config = config;
    skybox->config.environmentPath = mfStringDuplicate(config.environmentPath);
    skybox->renderer = renderer;
    skybox->isHdr = mfStringEndsWith(mfGetLogger(), config.environmentPath, ".hdr");

    {
        MFGpuImageConfig info = {
            .binding = config.binding,
            .generateMipmaps = false, // TODO: Later enable mipmaps for skybox
            .width = config.faceSize,
            .height = config.faceSize,
            .isCubemap = true,
            .imageFormat = skybox->isHdr ? MF_FORMAT_R32G32B32A32_SFLOAT : MF_FORMAT_R8G8B8A8_UNORM,
            .binding = 0
        };
        skybox->image = mfGpuImageCreate(renderer, info);
        if(config.generateIrradiance) {
            info.width = info.height = 32;
            skybox->irradiance = mfGpuImageCreate(renderer, info);
        }
    }

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
        skybox->layout = mfResourceSetLayoutCreate(1, &description, 2, renderer);

        skybox->set = mfResourceSetCreate(skybox->layout, renderer);

        {
            MFArray array = mfArrayCreate(mfGetLogger(), 1, sizeof(MFGpuImage*));
            mfArrayAddElement(array, MFGpuImage*, mfGetLogger(), skybox->image);
            mfResourceSetUpdate(skybox->set, &array, mfnull);
            mfArrayDestroy(&array, mfGetLogger());
        }
        
        if(config.generateIrradiance) {
            skybox->set2 = mfResourceSetCreate(skybox->layout, renderer);

            MFArray array = mfArrayCreate(mfGetLogger(), 1, sizeof(MFGpuImage*));
            mfArrayAddElement(array, MFGpuImage*, mfGetLogger(), skybox->irradiance);
            mfResourceSetUpdate(skybox->set2, &array, mfnull);
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
            .resourceLayouts = &skybox->layout,
            .cullMode = MF_CULL_MODE_NONE,
            .renderTarget = config.renderTarget
        };

        skybox->pipeline = mfPipelineCreate(renderer, &info);
    }

    SkyboxConvertEnvMapToSkybox(skybox, config, renderer);
    if(config.generateIrradiance) {
        SkyboxGenerateIrradiance(skybox, config, renderer);
    }

    skybox->init = true;
    return skybox;
}

void mfSkyboxDestroy(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    if(skybox->config.generateIrradiance) {
        mfResourceSetDestroy(skybox->set2);
        mfGpuImageDestroy(skybox->irradiance);
    }

    mfMeshDestroy(&skybox->mesh);
    mfPipelineDestroy(skybox->pipeline);
    mfResourceSetDestroy(skybox->set);
    mfResourceSetLayoutDestroy(skybox->layout);
    mfGpuImageDestroy(skybox->image);
    
    MF_FREEMEM(skybox->config.environmentPath);
    MF_SETMEM(skybox, 0, sizeof(MFSkybox));
    MF_FREEMEM(skybox);
}

size_t mfSkyboxGetSizeInBytes(void) {
    return sizeof(MFSkybox);
}

void mfSkyboxRender(MFSkybox* skybox, MFMat4 projection, MFMat4 view, bool irradiance) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");

    mfPipelineBind(skybox->pipeline, mfRendererGetViewport(skybox->renderer), mfRendererGetScissor(skybox->renderer));
    
    MFMat4 data[] = {
        projection,
        view
    };

    if(irradiance && skybox->config.generateIrradiance) {
        mfResourceSetBind(skybox->set2, skybox->pipeline);
    } else {
        mfResourceSetBind(skybox->set, skybox->pipeline);
    }
    mfPipelinePushConstant(skybox->pipeline, MF_SHADER_STAGE_VERTEX, 0, sizeof(MFMat4) * 2, data);
    mfMeshRender(&skybox->mesh);
}

MFGpuImage* mfSkyboxGetCubemapImage(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    return skybox->image;
}

MFGpuImage* mfSkyboxGetIrradianceCubemapImage(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");

    return skybox->irradiance;
}

#ifdef __cpluscplus
}
#endif