#include "mfrenderer.h"
#include "core/mftime.h"

#include "vk/backend.h"
#include "vk/render_target.h"

struct MFRenderer_s {
    VulkanBackend backend;
    f64 lastTime, deltaTime;
    MFTimer frameTimer;
};

void mfRendererInit(MFRenderer* renderer, const char* appName, b8 enableDepth, b8 vsync, b8 enableUI, MFWindow* window) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle provided shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MF_INFO(mfGetLogger(), "Creating the renderer");

    VulkanBackendConfig config = {
        .appName = appName,
        .vsync = vsync,
        .enableDepth = enableDepth,
        .enableUI = enableUI,
        .window = window
    };

    VulkanBackendInit(&renderer->backend, &config);
}

void mfRendererShutdown(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_INFO(mfGetLogger(), "Shutting down the renderer");
    
    VulkanBackendShutdown(&renderer->backend);

    MF_SETMEM(renderer, 0, sizeof(MFRenderer));
}

void mfRendererBeginframe(MFRenderer* renderer, MFWindow* window) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    f64 crntTime = mfGetTimeElapsed() * 1000; // s -> ms
    renderer->deltaTime = crntTime - renderer->lastTime;
    renderer->lastTime = crntTime;

    mfTimerStart(&renderer->frameTimer);

    VulkanBackendBeginframe(&renderer->backend, window);
}

void mfRendererEndframe(MFRenderer* renderer, MFWindow* window) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    mfTimerEnd(&renderer->frameTimer);

    VulkanBackendEndframe(&renderer->backend, window);
}

void mfRendererWait(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    vkDeviceWaitIdle(renderer->backend.ctx.device);
}

void mfRendererSetRenderTarget(MFRenderer* renderer, MFRenderTarget* rt) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    renderer->backend.renderTarget = rt;
}

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    renderer->backend.clearColor = (VkClearValue){.color = { color.r, color.g, color.b, 1.0f }};
}

void mfRendererDrawVertices(MFRenderer* renderer, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    VulkanBackendDrawVertices(&renderer->backend, vertexCount, instances, firstVertex, firstInstance);
}

void mfRendererDrawVerticesIndexed(MFRenderer* renderer, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    VulkanBackendDrawVerticesIndexed(&renderer->backend, indexCount, instances, firstIndex, firstInstance);
}

MFViewport mfRendererGetViewport(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    MFViewport vp = {
        .x = 0,
        .y = 0,
        .width = renderer->backend.ctx.swapchainExtent.width,
        .height = renderer->backend.ctx.swapchainExtent.height,
        .maxDepth = 1.0f,
        .minDepth = 0.0f
    };

    if(renderer->backend.renderTarget != mfnull) {
        vp.width = renderer->backend.renderTarget->images[0].info.width;
        vp.height = renderer->backend.renderTarget->images[0].info.height;
    }
    
    return vp;
}

MFRect2D mfRendererGetScissor(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFRect2D scissor = {
        .offsetX = 0,
        .offsetY = 0,
        .extentX = renderer->backend.ctx.swapchainExtent.width,
        .extentY = renderer->backend.ctx.swapchainExtent.height
    };

    if(renderer->backend.renderTarget != mfnull) {
        scissor.extentX = renderer->backend.renderTarget->images[0].info.width;
        scissor.extentY = renderer->backend.renderTarget->images[0].info.height;
    }

    return scissor;
}

void* mfRendererGetBackend(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    return (void*)&renderer->backend;
}

void* mfRendererGetPass(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    return (void*)renderer->backend.pass;
}

u8 mfRendererGetCurrentFrameIdx(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    return renderer->backend.frameIndex;
}

f64 mfRendererGetDeltaTime(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    
    return renderer->deltaTime;
}

f64 mfRendererGetFrameTime(MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    return renderer->frameTimer.delta;
}

u8 mfRendererGetFramesInFlight(void) {
    return FRAMES_IN_FLIGHT;
}

size_t mRendererGetSizeInBytes(void) {
    return sizeof(MFRenderer);
}
