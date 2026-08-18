#ifdef __cplusplus
extern "C" {
#endif

#include "mfrender_target.h"

#include "core/mfcore.h"

#include "mfrenderer.h"

#include "vk/common.h"
#include "vk/gpu_res.h"
#include "vk/backend.h"
#include "vk/image.h"
#include "vk/fb.h"
#include "vk/renderpass.h"
#include "vk/command_buffer.h"
#include "vk/render_target.h"
#include "vk/pipeline.h"

#include "mfpipeline.h"

#include <cimgui.h>
#include <cimgui_impl.h>

struct MFRenderTarget_s {
    MFRenderer* renderer;
    VulkanRenderTarget renderTarget;

    void* userData;
    void (*resizeCallback)(void* userData);

    bool init, begun;
};

MFRenderTarget* mfRenderTargetCreate(struct MFRenderer_s* renderer, bool hasDepth) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFRenderTarget* renderTarget = MF_ALLOCMEM(MFRenderTarget, sizeof(MFRenderTarget));

    renderTarget->renderer = renderer;
    renderTarget->resizeCallback = mfnull;

    renderTarget->begun = false;

    VulkanRenderTargetCreate(&renderTarget->renderTarget, renderer, hasDepth);

    renderTarget->init = true;
    return renderTarget;
}

void mfRenderTargetDestroy(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    VulkanRenderTargetDestroy(&renderTarget->renderTarget);
    
    MF_SETMEM(renderTarget, 0, sizeof(MFRenderTarget));
    MF_FREEMEM(renderTarget);
}

void mfRenderTargetResize(MFRenderTarget* renderTarget, MFVec2 extent) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    if(renderTarget->begun) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't resize the render target when the render target has already begun!");
        return;
    }

    if(extent.x == 0 || extent.y == 0) {
        return;
    }

    VulkanRenderTargetResize(&renderTarget->renderTarget, extent);

    if(renderTarget->resizeCallback != mfnull) {
        renderTarget->resizeCallback(renderTarget->userData);
    }
}

void mfRenderTargetSetClearColor(MFRenderTarget* renderTarget, MFVec3 color) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    renderTarget->renderTarget.clearValue = (VkClearValue){.color = {color.r, color.g, color.b, 1.0f}};
}

MFVec3 mfRenderTargetGetClearColor(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    float* color = renderTarget->renderTarget.clearValue.color.float32;

    return (MFVec3){ color[0], color[1], color[2] };
}

void mfRenderTargetBegin(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    MF_PANIC_IF(renderTarget->begun, mfGetLogger(), "The render target has already begun!");

    VulkanRenderTargetBegin(&renderTarget->renderTarget);

    renderTarget->begun = true;
}

void mfRenderTargetEnd(MFRenderTarget* renderTarget, bool waitOnCpu) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    MF_PANIC_IF(!renderTarget->begun, mfGetLogger(), "The render target hasn't begun yet!");

    VulkanRenderTargetEnd(&renderTarget->renderTarget, waitOnCpu);

    renderTarget->begun = false;
}

void mfRenderTargetSetResizeCallback(MFRenderTarget* renderTarget, void (*callback)(void* userData), void* userData) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    MF_PANIC_IF(userData == mfnull, mfGetLogger(), "The user data provided shouldn't be null!");
    MF_PANIC_IF(callback == mfnull, mfGetLogger(), "The resize callback func ptr provided shouldn't be null!");

    renderTarget->userData = userData;
    renderTarget->resizeCallback = callback;
}

void* mfRenderTargetGetPass(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    return (void*)renderTarget->renderTarget.renderPass.handle;
}

u8* mfRenderTargetGetCurrentImagePixels(MFRenderTarget* renderTarget, u32* width, u32* height) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    MF_PANIC_IF(width == mfnull, mfGetLogger(), "The width pointer provided shouldn't be null!");
    MF_PANIC_IF(height == mfnull, mfGetLogger(), "The height pointer provided shouldn't be null!");

    if(renderTarget->begun) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't get the pixels of render target when the render target has already begun!");
        return mfnull;
    }

    return VulkanImageGetPixels(&renderTarget->renderTarget.images[renderTarget->renderTarget.backend->frameIndex], 0, 0, width, height);
}

u32 mfRenderTargetGetWidth(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    return renderTarget->renderTarget.images[0].info.width;
}

u32 mfRenderTargetGetHeight(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    
    return renderTarget->renderTarget.images[0].info.height;
}

MFResourceSetLayout* mfRenderTargetGetResourceSetLayout(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    
    return renderTarget->renderTarget.layout;
}

void mfRenderTargetBindAttachmentResourceSets(MFRenderTarget* renderTarget, u64 setIndex, struct MFPipeline_s* pipeline) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    VulkanBackend* backend = renderTarget->renderTarget.backend;
    VulkanBackendCtx* ctx = &renderTarget->renderTarget.backend->ctx;

    VkCommandBuffer buff = backend->commandBuffers[backend->frameIndex];
    if(backend->renderTarget != mfnull) {
        buff = backend->renderTarget->commandBuffers[backend->frameIndex];
    }

    VulkanPipeline* pipelineBackend = (VulkanPipeline*)mfPipelineGetBackend(pipeline);

    vkCmdBindDescriptorSets(buff, pipelineBackend->bindPoint, pipelineBackend->layout, 
                                    setIndex, 1, &renderTarget->renderTarget.sets[backend->frameIndex], 
                                    0, mfnull);
}

ImTextureID mfRenderTargetGetColorAttachmentImTexID(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    
    if(!renderTarget->renderTarget.backend->config.enableUI)
        return mfnull;
    return (ImTextureID)renderTarget->renderTarget.igColorSets[renderTarget->renderTarget.backend->frameIndex];
}

void* mfRenderTargetGetBackend(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    return &renderTarget->renderTarget;
}

size_t mfRenderTargetGetSizeInBytes(void) {
    return sizeof(MFRenderTarget);
}

#ifdef __cplusplus
}
#endif
