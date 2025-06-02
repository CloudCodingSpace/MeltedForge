#include <mf.h>

#include <stb/stb_image.h>

#include "vertex.h"

typedef struct MFTState_s {
    MFPipeline* pipeline;
    MFGpuBuffer** ubos;
    MFGpuImage* tex;
    MFMesh mesh;
} MFTState;

typedef struct UBOData_s {
    MFMat4 proj;
    MFMat4 view;
    MFMat4 model;
} UBOData;

static void MFTOnInit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest init\n");

    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));
    
    MFTState* state = (MFTState*)pstate;
    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);
    
    // UBO & Mesh
    {
        state->ubos = MF_ALLOCMEM(MFGpuBuffer*, sizeof(MFGpuBuffer*) * mfGetRendererFramesInFlight());
        for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
            state->ubos[i] = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        }

        Vertex vertices[] = {
            {{ -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
            {{  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
            {{  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
            {{ -0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }},

            {{ -0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }, { 0.0f,0.0f }},
            {{  0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f }, { 1.0f,0.0f }},
            {{  0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f }, { 1.0f,1.0f }},
            {{ -0.5f,  0.5f, -0.5f}, { 1.0f, 1.0f, 1.0f }, { 0.0f,1.0f }}
        };

        u32 indices[] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4
        };

        mfMeshCreate(&state->mesh, appState->renderer, sizeof(vertices[0]) * MF_ARRAYLEN(vertices, Vertex), vertices, MF_ARRAYLEN(indices, u32), indices);

        MFGpuBufferConfig config = {
            .type = MF_GPU_BUFFER_TYPE_UBO,
            .size = sizeof(UBOData),
            .binding = 3,
            .stage = MF_SHADER_STAGE_VERTEX
        };

        UBOData uboData = {
            .proj = mfMat4Perspective(60.0f * MF_DEG2RAD_MULTIPLIER, (float)winConfig->width/(float)winConfig->height, 0.1f, 100.0f),
            .view = mfMat4LookAt((MFVec3){0.0f, 0.0f, 2.0f}, (MFVec3){0.0f, 0.0f, -1.0f}, (MFVec3){0.0f, 1.0f, 0.0f}),
            .model = mfMat4Identity()
        };

        for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
            mfGpuBufferAllocate(state->ubos[i], config, appState->renderer);
            mfGpuBufferUploadData(state->ubos[i], &uboData);
        }
    }
    // Image
    {
        u32 width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        u8* pixels = stbi_load("mfassets/logo.png", &width, &height, &channels, 4);
        if (!pixels) {
            slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- \n");
            slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, stbi_failure_reason());
            slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "\n");
            return;
        }

        state->tex = MF_ALLOCMEM(MFGpuImage, mfGetGpuImageSizeInBytes());
        
        MFGpuImageConfig config = {
            .width = width,
            .height = height,
            .pixels = pixels,
            .binding = 0
        };
        mfGpuImageCreate(state->tex, appState->renderer, config);

        stbi_image_free(pixels);
    }
    // Pipeline
    {
        u32 attribCount = 0, bindingCount = 1;
        MFVertexInputAttributeDescription* attribDescs = getVertAttribDescs(&attribCount);
        MFVertexInputBindingDescription bindingDesc = getVertBindingDesc();

        u32 imageCount = 1;
        MFGpuImage* images[] = {
            state->tex
        };

        MFPipelineConfig info = {
            .extent = (MFVec2){ .x = winConfig->width, .y = winConfig->height },
            .hasDepth = true,
            .vertPath = "shaders/default.vert.spv",
            .fragPath = "shaders/default.frag.spv",
            .attribDescsCount = attribCount,
            .attribDescs = attribDescs,
            .bindingDescsCount = bindingCount,
            .bindingDescs = &bindingDesc,
            .imgCount = imageCount,
            .images = images,
            .buffCount = mfGetRendererFramesInFlight(),
            .buffers = state->ubos
        };
        mfPipelineInit(state->pipeline, appState->renderer, &info);

        MF_FREEMEM(attribDescs);
    }
}

static void MFTOnDeinit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest deinit\n");
    MFTState* state = (MFTState*)pstate;
    
    mfGpuImageDestroy(state->tex);
    
    for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
        mfGpuBufferFree(state->ubos[i]);
        MF_FREEMEM(state->ubos[i]);
    }

    mfMeshDestroy(&state->mesh);
    mfPipelineDestroy(state->pipeline);
    
    MF_FREEMEM(state->ubos);
    MF_FREEMEM(state->pipeline);
    MF_FREEMEM(state->tex);
}

static void MFTOnRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);

    mfPipelineBind(state->pipeline, mfRendererGetViewport(winConfig), mfRendererGetScissor(winConfig));
    mfMeshRender(&state->mesh);
}

static void MFTOnUpdate(void* pstate, void* pappState) {
    MFDefaultAppState* aState = (MFDefaultAppState*)pappState;
    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* winConfig = mfGetWindowConfig(aState->window);

    UBOData uboData = {
        .proj = mfMat4Perspective(60.0f * MF_DEG2RAD_MULTIPLIER, (float)winConfig->width/(float)winConfig->height, 0.1f, 100.0f),
        .view = mfMat4LookAt((MFVec3){0.0f, 0.0f, 2.0f}, (MFVec3){0.0f, 0.0f, -1.0f}, (MFVec3){0.0f, 1.0f, 0.0f}),
        .model = mfMat4Mul(mfMat4RotateX(mfGetCurrentTime() * 90.0f * MF_DEG2RAD_MULTIPLIER), mfMat4RotateZ(mfGetCurrentTime() * 90.0f * MF_DEG2RAD_MULTIPLIER))
    };

    mfGpuBufferUploadData(state->ubos[mfGetRendererCurrentFrameIdx(aState->renderer)], &uboData);

    if(mfInputIsKeyPressed(aState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(aState->window);
    }
}

MFAppConfig mfClientCreateAppConfig() {
    MFAppConfig config = mfCreateDefaultApp("MFTest");
    
    config.layerCount = 1;
    config.layers = MF_ALLOCMEM(MFLayer, sizeof(MFLayer) * config.layerCount);
    config.layers[0] = (MFLayer){
        .state = MF_ALLOCMEM(MFTState, sizeof(MFTState)),
        .onInit = &MFTOnInit,
        .onDeinit = &MFTOnDeinit,
        .onRender = &MFTOnRender,
        .onUpdate = &MFTOnUpdate
    };
    config.winConfig.resizable = true;
    
    return config;
}