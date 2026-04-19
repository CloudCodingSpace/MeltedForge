#ifdef __cplusplus
extern "C" {
#endif

#include "mfapp.h"
#include "mfprofiler.h"
#include "systems/mfmaterial_system.h"

#include <stb/stb_image.h>

static void initApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;

    state->window = MF_ALLOCMEM(MFWindow, mfWindowGetSizeInBytes());
    mfWindowInit(state->window, config->winConfig);

    // Setting icon
    {
        u32 width, height, channels;
        u8* data = stbi_load(MF_WINDOW_DEFAULT_ICON_PATH, &width, &height, &channels, 4);
        mfWindowSetIcon(state->window, width, height, data);
        stbi_image_free(data);
    }

    state->renderer = MF_ALLOCMEM(MFRenderer, mfRendererGetSizeInBytes());
    mfRendererInit(state->renderer, config->appName, config->enableDepth, config->vsync, config->enableUI, state->window);

    mfMaterialSystemInitialize();

    for(u32 i = 0; i < config->layers.len; i++) {
        MFLayer* layer = &mfArrayGetElement(config->layers, MFLayer, i);
        if(layer->onInit)
            layer->onInit(layer->state, st);
    }
}

static void deinitApp(void* st, MFAppConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;
    mfRendererWaitForGPU(state->renderer);

    for(u32 i = 0; i < config->layers.len; i++) {
        MFLayer* layer = &mfArrayGetElement(config->layers, MFLayer, i);
        if(layer->onDeinit)
            layer->onDeinit(layer->state, st);
        if(layer->state != 0)
            MF_FREEMEM(layer->state);
    }
    
    mfMaterialSystemShutdown();

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
        if(mfRendererBeginframe(state->renderer, state->window)) {
            for(u32 i = 0; i < config->layers.len; i++) {
                MFLayer* layer = &mfArrayGetElement(config->layers, MFLayer, i);
                if(layer->onUpdate)
                    layer->onUpdate(layer->state, st);
            }

            for(u32 i = 0; i < config->layers.len; i++) {
                MFLayer* layer = &mfArrayGetElement(config->layers, MFLayer, i);
                if(layer->onRender)
                    layer->onRender(layer->state, st);
                if(config->enableUI && layer->onUIRender)
                    layer->onUIRender(layer->state, st);
            }
            mfRendererEndframe(state->renderer, state->window);
        }
        mfWindowUpdate(state->window);

        mfProfilerMarkFrame();
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
            .title = (name) ? name : "MeltedForge application",
            .width = 800,
            .height = 600
        },
        .initApp = &initApp,
        .shutdownApp = &deinitApp,
        .runApp = &runApp,
        .getWindowHandle = &getWindow,
        .appName = (name) ? name : "MeltedForge application"
    };
}

#ifdef __cplusplus
}
#endif