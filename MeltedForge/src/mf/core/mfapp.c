#include "mfapp.h"

static void initApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;
    state->window = MF_ALLOCMEM(MFWindow, mfWindowGetSizeInBytes());
    mfWindowInit(state->window, config->winConfig);

    mfWindowSetIcon(state->window, MF_WINDOW_DEFAULT_ICON_PATH);

    state->renderer = MF_ALLOCMEM(MFRenderer, mfGetRendererSizeInBytes());
    mfRendererInit(state->renderer, config->name, config->vsync, config->enableUI, state->window);

    for(u32 i = 0; i < config->layers.len; i++) {
        MFLayer* layer = &mfArrayGet(config->layers, MFLayer, i);
        if(layer->onInit)
            layer->onInit(layer->state, st);
    }
}

static void deinitApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;
    mfRendererWait(state->renderer);

    for(u32 i = 0; i < config->layers.len; i++) {
        MFLayer* layer = &mfArrayGet(config->layers, MFLayer, i);
        if(layer->onDeinit)
            layer->onDeinit(layer->state, st);
        if(layer->state != 0)
            MF_FREEMEM(layer->state);
    }
    
    if(config->layers.data && config->layers.len > 0)
        mfArrayDestroy(&config->layers, mfGetLogger());

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
        for(u32 i = 0; i < config->layers.len; i++) {
            MFLayer* layer = &mfArrayGet(config->layers, MFLayer, i);
            if(layer->onRender)
                layer->onRender(layer->state, st);
            if(config->enableUI && layer->onUIRender)
                layer->onUIRender(layer->state, st);
        }
        mfRendererEndframe(state->renderer, state->window);

        for(u32 i = 0; i < config->layers.len; i++) {
            MFLayer* layer = &mfArrayGet(config->layers, MFLayer, i);
            if(layer->onUpdate)
                layer->onUpdate(layer->state, st);
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
