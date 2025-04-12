#include <mf.h>

typedef struct MFTestLayerState_s {

} MFTestLayerState;

static void MFTOnInit(void* state, void* appState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest init\n");
}

static void MFTOnDeinit(void* state, void* appState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest deinit\n");
}

static void MFTOnRender(void* state, void* appState) {

}

static void MFTOnUpdate(void* state, void* appState) {

}

MFAppConfig mfClientCreateAppConfig() {
    MFAppConfig config = mfCreateDefaultApp("MFTest");
    
    config.layerCount = 1;
    config.layers = MF_ALLOCMEM(MFLayer, sizeof(MFLayer) * config.layerCount);
    config.layers[0] = (MFLayer){
        .state = MF_ALLOCMEM(MFTestLayerState, sizeof(MFTestLayerState)),
        .onInit = &MFTOnInit,
        .onDeinit = &MFTOnDeinit,
        .onRender = &MFTOnRender,
        .onUpdate = &MFTOnUpdate
    };
    
    return config;
}