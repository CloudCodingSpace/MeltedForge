#pragma once

#include "window/mfwindow.h"

typedef struct MFLayer_s {
    void* state;
    void (*onInit)(void* state, void* appState);
    void (*onDeinit)(void* state, void* appState);
    void (*onRender)(void* state, void* appState);
    void (*onUpdate)(void* state, void* appState);
} MFLayer;

typedef struct MFAppConfig_s {
    void* state;
    MFWindowConfig winConfig;
    u32 layerCount;
    MFLayer* layers;
    void (*initApp)(void* state, struct MFAppConfig_s*);
    void (*shutdownApp)(void* state, struct MFAppConfig_s*);
    void (*runApp)(void* state, struct MFAppConfig_s*);
    MFWindow* (*getWindowHandle)(void* state);
} MFAppConfig;

typedef struct MFDefaultAppState_s {
    MFWindow* window;
} MFDefaultAppState;

MFAppConfig mfCreateDefaultApp();