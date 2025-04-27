#include "mfrenderer.h"

#include "vk/backend.h"

struct MFRenderer_s {
    VulkanBackend backend;
};

void mfRendererInit(MFRenderer* renderer, const char* appName, MFWindow* window) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MF_INFO(mfGetLogger(), "Creating the renderer\n");

    VulkanBckndInit(&renderer->backend, appName, window);
}

void mfRendererShutdown(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_INFO(mfGetLogger(), "Shutting down the renderer\n");
    
    VulkanBckndShutdown(&renderer->backend);

    MF_SETMEM(renderer, 0, sizeof(MFRenderer));
}

void mfRendererBeginframe(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    VulkanBckndBeginframe(&renderer->backend);
}

void mfRendererEndframe(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    VulkanBckndEndframe(&renderer->backend);
}

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    renderer->backend.clearColor = (VkClearValue){.color = { color.r, color.g, color.b, 1.0f }};
}

size_t mfGetRendererSizeInBytes() {
    return sizeof(MFRenderer);
}