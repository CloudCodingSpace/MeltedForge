#include <mf.h>

static void MFTOnInit(void* state, void* appState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest init\n");

    MFDefaultAppState* as = (MFDefaultAppState*) appState;
    mfRendererSetClearColor(as->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));
}

static void MFTOnDeinit(void* state, void* appState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest deinit\n");
}

static void MFTOnRender(void* state, void* appState) {

}

static void MFTOnUpdate(void* state, void* appState) {
    MFDefaultAppState* aState = (MFDefaultAppState*)appState;
    if(mfInputIsKeyPressed(aState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(aState->window);
    }
}

MFAppConfig mfClientCreateAppConfig() {
    MFAppConfig config = mfCreateDefaultApp("MFTest");
    
    config.layerCount = 1;
    config.layers = MF_ALLOCMEM(MFLayer, sizeof(MFLayer) * config.layerCount);
    config.layers[0] = (MFLayer){
        .state = mfnull,
        .onInit = &MFTOnInit,
        .onDeinit = &MFTOnDeinit,
        .onRender = &MFTOnRender,
        .onUpdate = &MFTOnUpdate
    };
    config.winConfig.resizable = true;
    
    return config;
}