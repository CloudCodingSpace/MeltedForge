#include "mftest.h"

#define MF_INCLUDE_ENTRY
#include <mf.h>

MFAppConfig mfClientCreateAppConfig() {
    MFAppConfig config = mfCreateDefaultApp("MFTest");

    MFLayer testLayer = {
        .state = MF_ALLOCMEM(MFTState, sizeof(MFTState)),
        .onInit = &MFTOnInit,
        .onDeinit = &MFTOnDeinit,
        .onRender = &MFTOnRender,
        .onUpdate = &MFTOnUpdate,
        .onUIRender = &MFTOnUIRender
    };

    MFArray layers = mfArrayCreate(1, sizeof(MFLayer));
    mfArrayAddElement(&layers, MFLayer, testLayer);

    config.layers = layers;
    config.winConfig.resizable = true;
    config.vsync = false;
    config.enableUI = true;
    config.enableDepth = true;

    return config;
}
