#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "window/mfwindow.h"
#include "renderer/mfrenderer.h"

#include "mfarray.h"

typedef struct MFLayer_s {
    void* state;
    void (*onInit)(void* state, void* appState);
    void (*onDeinit)(void* state, void* appState);
    void (*onRender)(void* state, void* appState);
    void (*onUpdate)(void* state, void* appState);
    void (*onUIRender)(void* state, void* appState);
} MFLayer;

typedef struct MFAppConfig_s {
    const char* appName;
    void* state;
    bool vsync, enableUI, enableDepth;
    MFWindowConfig winConfig;
    MFArray layers;

    void (*initApp)(void* state, struct MFAppConfig_s*);
    void (*shutdownApp)(void* state, struct MFAppConfig_s*);
    void (*runApp)(void* state, struct MFAppConfig_s*);
    MFWindow* (*getWindowHandle)(void* state);
} MFAppConfig;

typedef struct MFDefaultAppState_s {
    MFWindow* window;
    MFRenderer* renderer;
} MFDefaultAppState;

MFAppConfig mfCreateDefaultApp(const char* name);

#ifdef __cplusplus
}
#endif