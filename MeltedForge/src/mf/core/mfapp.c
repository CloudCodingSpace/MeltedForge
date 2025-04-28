#include "mfapp.h"

static void initApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;
    state->window = MF_ALLOCMEM(MFWindow, mfWindowGetSizeInBytes());
    mfWindowInit(state->window, config->winConfig);

    state->renderer = MF_ALLOCMEM(MFRenderer, mfGetRendererSizeInBytes());
    mfRendererInit(state->renderer, config->name, state->window);

    for(u32 i = 0; i < config->layerCount; i++) {
        config->layers[i].onInit(config->layers[i].state, st);
    }
}

static void deinitApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;

    for(u32 i = 0; i < config->layerCount; i++) {
        config->layers[i].onDeinit(config->layers[i].state, st);
        if(config->layers[i].state != 0)
            MF_FREEMEM(config->layers[i].state);
    }
    
    if(config->layers && config->layerCount > 0)
        MF_FREEMEM(config->layers);

    mfRendererShutdown(state->renderer);
    mfWindowDestroy(state->window);
    MF_FREEMEM(state->renderer);
    MF_FREEMEM(state->window);
}

static void runApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;

    mfWindowShow(state->window);
    while(mfIsWindowOpen(state->window)) {
        mfRendererBeginframe(state->renderer, state->window);
        for(u32 i = 0; i < config->layerCount; i++) {
            config->layers[i].onRender(config->layers[i].state, st);
        }
        mfRendererEndframe(state->renderer, state->window);

        for(u32 i = 0; i < config->layerCount; i++) {
            config->layers[i].onUpdate(config->layers[i].state, st);
        }
        mfWindowUpdate(state->window);
    }
}

static MFWindow* getWindow(void* state) {
    return ((MFDefaultAppState*)state)->window;
}

MFAppConfig mfCreateDefaultApp(const char* name) {
    return (MFAppConfig) {
        .state = MF_ALLOCMEM(MFDefaultAppState, sizeof(MFDefaultAppState)),
        .winConfig = (MFWindowConfig){
            .centered = true,
            .fullscreen = false,
            .resizable = false,
            .title = name,
            .width = 800,
            .height = 600
        },
        .initApp = &initApp,
        .shutdownApp = &deinitApp,
        .runApp = &runApp,
        .getWindowHandle = &getWindow,
        .name = name
    };
}