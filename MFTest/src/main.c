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

    MFArray layers = mfArrayCreate(mfGetLogger(), 1, sizeof(MFLayer));
    mfArrayAddElement(layers, MFLayer, mfGetLogger(), testLayer);
   
    config.layers = layers;
    config.winConfig.resizable = true;
    config.vsync = false;
    config.enableUI = true;

    return config;
}
