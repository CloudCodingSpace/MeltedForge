#include "backend.h"

#include "cmd.h"
#include "renderpass.h"
#include "fb.h"

#include <GLFW/glfw3.h>

void OnResize(VulkanBackend* backend, u32 width, u32 height, MFWindow* window) {
    if(backend->ctx.scExtent.width == width && backend->ctx.scExtent.height == height)
        return;
    if(width == 0 && height == 0)
        return;

    VK_CHECK(vkDeviceWaitIdle(backend->ctx.device));

    for(u32 i = 0; i < backend->fbCount; i++) {
        VulkanFbDestroy(&backend->ctx, backend->fbs[i]);
    }

    VulkanBckndCtxResize(&backend->ctx, width, height, window);

    for(u32 i = 0; i < backend->fbCount; i++) {
        VkImageView views[] = {
            backend->ctx.scImgViews[i],
            backend->ctx.depthImage.view
        };

        backend->fbs[i] = VulkanFbCreate(&backend->ctx, backend->pass, MF_ARRAYLEN(views, VkImageView), views, backend->ctx.scExtent); 
    }
}

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window) {
    VulkanBckndCtxInit(&backend->ctx, appName, window);

    backend->cmdPool = VulkanCommandPoolCreate(&backend->ctx, backend->ctx.qData.gQueueIdx);
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        backend->cmdBuffers[i] = VulkanCommandBufferAllocate(&backend->ctx, backend->cmdPool, true);
    }

    backend->pass = VulkanRenderPassCreate(&backend->ctx, backend->ctx.scFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, true);
    
    // Framebuffers
    backend->fbCount = backend->ctx.scImgCount;
    backend->fbs = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * backend->fbCount);
    for(u32 i = 0; i < backend->fbCount; i++) {
        VkImageView views[] = {
            backend->ctx.scImgViews[i],
            backend->ctx.depthImage.view
        };

        backend->fbs[i] = VulkanFbCreate(&backend->ctx, backend->pass, MF_ARRAYLEN(views, VkImageView), views, backend->ctx.scExtent); 
    }
    
    // Sync objs
    {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo semaInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
            };

            VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };

            VK_CHECK(vkCreateSemaphore(backend->ctx.device, &semaInfo, backend->ctx.allocator, &backend->imgAvailableSemas[i]));
            VK_CHECK(vkCreateSemaphore(backend->ctx.device, &semaInfo, backend->ctx.allocator, &backend->rndrFinishedSemas[i]));
            VK_CHECK(vkCreateFence(backend->ctx.device, &fenceInfo, backend->ctx.allocator, &backend->inFlightFences[i]));
        }
    }

    backend->crntFrmIdx = 0;
}

void VulkanBckndShutdown(VulkanBackend* backend) {
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(backend->ctx.device, backend->imgAvailableSemas[i], backend->ctx.allocator);
        vkDestroySemaphore(backend->ctx.device, backend->rndrFinishedSemas[i], backend->ctx.allocator);
        vkDestroyFence(backend->ctx.device, backend->inFlightFences[i], backend->ctx.allocator);
    }

    for(u32 i = 0; i < backend->fbCount; i++) {
        VulkanFbDestroy(&backend->ctx, backend->fbs[i]);
    }
    
    VulkanRenderPassDestroy(&backend->ctx, backend->pass);
    VulkanCommandPoolDestroy(&backend->ctx, backend->cmdPool);
    VulkanBckndCtxDestroy(&backend->ctx);

    MF_FREEMEM(backend->fbs);
    MF_SETMEM(backend, 0, sizeof(VulkanBackend));
}

void VulkanBckndBeginframe(VulkanBackend* backend, MFWindow* window) {
    VK_CHECK(vkWaitForFences(backend->ctx.device, 1, &backend->inFlightFences[backend->crntFrmIdx], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(backend->ctx.device, 1, &backend->inFlightFences[backend->crntFrmIdx]));

    VkResult result = vkAcquireNextImageKHR(backend->ctx.device, backend->ctx.swapchain, UINT64_MAX, backend->imgAvailableSemas[backend->crntFrmIdx], VK_NULL_HANDLE, &backend->scImgIdx);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        OnResize(backend, (u32)mfGetWindowConfig(window)->width, (u32)mfGetWindowConfig(window)->height, window);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK(result);
    }

    VK_CHECK(vkResetCommandBuffer(backend->cmdBuffers[backend->crntFrmIdx], 0));
    VulkanCommandBufferBegin(backend->cmdBuffers[backend->crntFrmIdx]);

    VkClearValue values[2] = {
        backend->clearColor
    };
    values[1].depthStencil.depth = 1.0f;
    values[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rpInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = MF_ARRAYLEN(values, VkClearColorValue),
        .pClearValues = values,
        .framebuffer = backend->fbs[backend->scImgIdx],
        .renderArea = (VkRect2D){.extent = backend->ctx.scExtent, .offset = (VkOffset2D){ 0, 0 }},
        .renderPass = backend->pass
    };

    vkCmdBeginRenderPass(backend->cmdBuffers[backend->crntFrmIdx], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanBckndEndframe(VulkanBackend* backend, MFWindow* window) {
    vkCmdEndRenderPass(backend->cmdBuffers[backend->crntFrmIdx]);
    VulkanCommandBufferEnd(backend->cmdBuffers[backend->crntFrmIdx]);

    VkPipelineStageFlags waitDstMask[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &backend->cmdBuffers[backend->crntFrmIdx],
        .pWaitDstStageMask = waitDstMask,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &backend->rndrFinishedSemas[backend->crntFrmIdx],
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &backend->imgAvailableSemas[backend->crntFrmIdx]
    };

    VK_CHECK(vkQueueSubmit(backend->ctx.qData.gQueue, 1, &submitInfo, backend->inFlightFences[backend->crntFrmIdx]));

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &backend->scImgIdx,
        .swapchainCount = 1,
        .pSwapchains = &backend->ctx.swapchain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &backend->rndrFinishedSemas[backend->crntFrmIdx]
    };

    VkResult result = vkQueuePresentKHR(backend->ctx.qData.pQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        OnResize(backend, (u32)mfGetWindowConfig(window)->width, (u32)mfGetWindowConfig(window)->height, window);
        return;
    }
    VK_CHECK(result);

    backend->crntFrmIdx = (backend->crntFrmIdx + 1) % FRAMES_IN_FLIGHT;
}