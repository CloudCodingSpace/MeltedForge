#include "mfapp.h"

typedef struct MFDefaultAppState_s {
    MFWindow* window;
} MFDefaultAppState;

static void initApp(void* st, MFWindowConfig* config) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;
    state->window = MF_ALLOCMEM(MFWindow, mfWindowGetSizeInBytes());
    mfWindowInit(state->window, *config);
}

static void deinitApp(void* st) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;

    mfWindowDestroy(state->window);
    MF_FREEMEM(state->window);
}
static void runApp(void* st) {
    MFDefaultAppState* state = (MFDefaultAppState*) st;

    mfWindowShow(state->window);
    while(mfIsWindowOpen(state->window)) {


        mfWindowUpdate(state->window);
    }
}

MFAppConfig mfCreateDefaultApp() {
    return (MFAppConfig) {
        .state = MF_ALLOCMEM(MFDefaultAppState, sizeof(MFDefaultAppState)),
        .winConfig = (MFWindowConfig){
            .centered = true,
            .fullscreen = false,
            .resizable = false,
            .title = "MFTest",
            .width = 800,
            .height = 600
        },
        .initApp = &initApp,
        .shutdownApp = &deinitApp,
        .runApp = &runApp
    };
}