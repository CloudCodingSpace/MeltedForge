#include "mfrenderer.h"

#include "vk/backend.h"

struct MFRenderer_s {
    MFVkBackend backend;
};

void mfRendererInit(MFRenderer* renderer, const char* appName, MFWindow* window) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MF_INFO(mfGetLogger(), "Creating the renderer\n");

    mfVkBckndInit(&renderer->backend, appName, window);
}

void mfRendererShutdown(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_INFO(mfGetLogger(), "Shutting down the renderer\n");
    
    mfVkBckndShutdown(&renderer->backend);
}

void mfRendererBeginframe(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    mfVkBckndBeginframe(&renderer->backend);
}

void mfRendererEndframe(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    mfVkBckndEndframe(&renderer->backend);
}

size_t mfGetRendererSizeInBytes() {
    return sizeof(MFRenderer);
}