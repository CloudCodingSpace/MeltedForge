#include <mf.h>

#include <stb/stb_image.h>

#include "vertex.h"

typedef struct MFTState_s {
    MFPipeline* pipeline;
    MFGpuBuffer* vertexBuffer;
    MFGpuBuffer* indexBuffer;
    MFGpuImage* tex;
} MFTState;

static void MFTOnInit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest init\n");

    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));
    
    MFTState* state = (MFTState*)pstate;
    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);
    
    // Buffers
    {
        state->vertexBuffer = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        state->indexBuffer = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
    
        Vertex vertices[] = {
            {{ -0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{  0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{  0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ -0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
        };

        u32 indices[] = {
            0, 1, 2,
            2, 3, 0 
        };

        MFGpuBufferConfig config = {
            .data = vertices,
            .size = sizeof(vertices[0]) * MF_ARRAYLEN(vertices, Vertex),
            .type = MF_GPU_BUFFER_TYPE_VERTEX
        };

        mfGpuBufferAllocate(state->vertexBuffer, config, appState->renderer);
        
        config.data = indices;
        config.size = sizeof(indices[0]) * MF_ARRAYLEN(indices, u32);
        config.type = MF_GPU_BUFFER_TYPE_INDEX;
        
        mfGpuBufferAllocate(state->indexBuffer, config, appState->renderer);
    }
    // Image
    {
        u32 width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        u8* pixels = stbi_load("mfassets/logo.png", &width, &height, &channels, 4);

        state->tex = MF_ALLOCMEM(MFGpuImage, mfGetGpuImageSizeInBytes());
        
        MFGpuImageConfig config = {
            .width = width,
            .height = height,
            .pixels = pixels
        };
        mfGpuImageCreate(state->tex, appState->renderer, config);

        stbi_image_free(pixels);
    }
    // Pipeline
    {
        u32 attribCount = 0, bindingCount = 1;
        MFVertexInputAttributeDescription* attribDescs = getVertAttribDescs(&attribCount);
        MFVertexInputBindingDescription bindingDesc = getVertBindingDesc();

        u32 resources = 1;
        MFResourceDesc descs[] = {
            mfGetGpuImageDescription(0)
        };

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
            .resourceDescCount = resources,
            .resourceDescs = descs,
            .imgCount = imageCount,
            .images = images
        };
        mfPipelineInit(state->pipeline, appState->renderer, &info);

        MF_FREEMEM(attribDescs);
    }
}

static void MFTOnDeinit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest deinit\n");
    MFTState* state = (MFTState*)pstate;
    
    mfGpuImageDestroy(state->tex);
    mfGpuBufferFree(state->indexBuffer);
    mfGpuBufferFree(state->vertexBuffer);
    mfPipelineDestroy(state->pipeline);
    
    MF_FREEMEM(state->vertexBuffer);
    MF_FREEMEM(state->indexBuffer);
    MF_FREEMEM(state->pipeline);
    MF_FREEMEM(state->tex);
}

static void MFTOnRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);

    mfPipelineBind(state->pipeline, mfRendererGetViewport(winConfig), mfRendererGetScissor(winConfig));
    mfGpuBufferBind(state->vertexBuffer);
    mfGpuBufferBind(state->indexBuffer);
    mfRendererDrawVerticesIndexed(appState->renderer, 6, 1, 0, 0);
}

static void MFTOnUpdate(void* pstate, void* pappState) {
    MFDefaultAppState* aState = (MFDefaultAppState*)pappState;
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