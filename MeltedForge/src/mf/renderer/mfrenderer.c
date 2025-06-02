#include "mfrenderer.h"
#include "core/mftimer.h"

#include "vk/backend.h"

struct MFRenderer_s {
    VulkanBackend backend;
    f64 lastTime, deltaTime;
    MFTimer frameTimer;
};

void mfRendererInit(MFRenderer* renderer, const char* appName, b8 vsync, MFWindow* window) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MF_INFO(mfGetLogger(), "Creating the renderer\n");

    VulkanBckndInit(&renderer->backend, appName, vsync, window);
}

void mfRendererShutdown(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_INFO(mfGetLogger(), "Shutting down the renderer\n");
    
    VulkanBckndShutdown(&renderer->backend);

    MF_SETMEM(renderer, 0, sizeof(MFRenderer));
}

void mfRendererBeginframe(MFRenderer* renderer, MFWindow* window) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    f64 crntTime = mfGetCurrentTime() * 1000; // s -> ms
    renderer->deltaTime = crntTime - renderer->lastTime;
    renderer->lastTime = crntTime;

    mfTimerStart(&renderer->frameTimer);

    VulkanBckndBeginframe(&renderer->backend, window);
}

void mfRendererEndframe(MFRenderer* renderer, MFWindow* window) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    mfTimerEnd(&renderer->frameTimer);

    VulkanBckndEndframe(&renderer->backend, window);
}

void mfRendererWait(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    vkDeviceWaitIdle(renderer->backend.ctx.device);
}

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    renderer->backend.clearColor = (VkClearValue){.color = { color.r, color.g, color.b, 1.0f }};
}

void mfRendererDrawVertices(MFRenderer* renderer, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    VulkanBackendDrawVertices(&renderer->backend, vertexCount, instances, firstVertex, firstInstance);
}

void mfRendererDrawVerticesIndexed(MFRenderer* renderer, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    VulkanBackendDrawVerticesIndexed(&renderer->backend, indexCount, instances, firstIndex, firstInstance);
}

MFViewport mfRendererGetViewport(const MFWindowConfig* config) {
    MF_ASSERT(config == mfnull, mfGetLogger(), "The window config handle provided shouldn't be null!");

    MFViewport vp = {
        .x = 0,
        .y = 0,
        .width = config->width,
        .height = config->height,
        .maxDepth = 1.0f,
        .minDepth = 0.0f
    };

    return vp;
}

MFRect2D mfRendererGetScissor(const MFWindowConfig* config) {
    MF_ASSERT(config == mfnull, mfGetLogger(), "The window config handle provided shouldn't be null!");

    MFRect2D scissor = {
        .offsetX = 0,
        .offsetY = 0,
        .extentX = config->width,
        .extentY = config->height
    };

    return scissor;
}

void* mfRendererGetBackend(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    return (void*)&renderer->backend;
}

u8 mfGetRendererCurrentFrameIdx(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    return renderer->backend.crntFrmIdx;
}

f64 mfGetRendererGetDeltaTime(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    return renderer->deltaTime;
}

f64 mfGetRendererGetFrameTime(MFRenderer* renderer) {
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    return renderer->frameTimer.delta;
}

u8 mfGetRendererFramesInFlight() {
    return FRAMES_IN_FLIGHT;
}

size_t mfGetRendererSizeInBytes() {
    return sizeof(MFRenderer);
}