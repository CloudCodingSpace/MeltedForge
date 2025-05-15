#include <mf.h>

typedef struct MFTState_s {
    MFPipeline* pipeline;
} MFTState;

static void MFTOnInit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest init\n");

    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));
    
    MFTState* state = (MFTState*)pstate;
    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);
    
    // Pipeline
    {
        MFPipelineInfo info = {
            .extent = (MFVec2){ .x = winConfig->width, .y = winConfig->height },
            .hasDepth = true,
            .vertPath = "shaders/default.vert.spv",
            .fragPath = "shaders/default.frag.spv"
        };
        mfPipelineInit(state->pipeline, appState->renderer, &info);
    }
}

static void MFTOnDeinit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest deinit\n");
    MFTState* state = (MFTState*)pstate;
    
    mfPipelineDestroy(state->pipeline);
    
    MF_FREEMEM(state->pipeline);
}

static void MFTOnRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);

    mfPipelineBind(state->pipeline, mfRendererGetViewport(winConfig), mfRendererGetScissor(winConfig));
    mfRendererDrawVertices(appState->renderer, 3, 1, 0, 0);
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