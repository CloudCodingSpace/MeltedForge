#include "backend.h"

#include "command_buffer.h"
#include "renderpass.h"
#include "fb.h"

#include <GLFW/glfw3.h>

#include <stdbool.h>

#include <cimgui.h>
#include <cimgui_impl.h>

#include "../mfrender_target.h"
#include "render_target.h"

void OnResize(VulkanBackend* backend, u32 width, u32 height, MFWindow* window) {
    if(backend->ctx.scExtent.width == width && backend->ctx.scExtent.height == height)
        return;
    if(width == 0 && height == 0)
        return;

    VK_CHECK(vkDeviceWaitIdle(backend->ctx.device));

    for(u32 i = 0; i < backend->fbCount; i++) {
        VulkanFbDestroy(&backend->ctx, backend->fbs[i]);
    }

    VulkanBackendCtxResize(&backend->ctx, width, height, window);

    for(u32 i = 0; i < backend->fbCount; i++) {
        u32 len = 1;
        VkImageView views[2] = {
            backend->ctx.scImgViews[i]
        };
        if(backend->enableDepth) {
            len++;
            views[1] = backend->ctx.depthImage.view;
        }

        backend->fbs[i] = VulkanFbCreate(&backend->ctx, backend->pass, len, views, backend->ctx.scExtent); 
    }
}

void VulkanBackendInit(VulkanBackend* backend, VulkanBackendConfig* config) {
    backend->enableUI = config->enableUI;
    backend->enableDepth = config->enableDepth;

    VulkanBackendCtxInit(&backend->ctx, config->appName, config->vsync, config->enableDepth, config->window);

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        backend->cmdBuffers[i] = VulkanCommandBufferAllocate(&backend->ctx, backend->ctx.cmdPool, true);
    }

    {
        VulkanRenderPassInfo info = {
            .format = backend->ctx.scFormat.format,
            .initiaLay = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLay = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .hasDepth = config->enableDepth,
            .renderTarget = false
        };

        backend->pass = VulkanRenderPassCreate(&backend->ctx, info);
    }
    
    // Framebuffers
    backend->fbCount = backend->ctx.scImgCount;
    backend->fbs = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * backend->fbCount);
    for(u32 i = 0; i < backend->fbCount; i++) {
        u32 len = 1;
        VkImageView views[2] = {
            backend->ctx.scImgViews[i]
        };
        if(config->enableDepth) {
            len++;
            views[1] = backend->ctx.depthImage.view;
        }

        backend->fbs[i] = VulkanFbCreate(&backend->ctx, backend->pass, len, views, backend->ctx.scExtent); 
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

    backend->frameIndex = 0;

    if(!config->enableUI)
        return;

    // UI
    {
        ImGuiContext* ctx = igCreateContext(NULL);
        igSetCurrentContext(ctx);

        ImGuiIO* io = igGetIO_Nil();
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        
        ImGuiStyle* style = igGetStyle();
        style->WindowPadding = (ImVec2){0, 0};

        ImFontAtlas_AddFontFromFileTTF(io->Fonts, "mfassets/fonts/consolas.ttf", 18.0f, mfnull, mfnull);

        ImGui_ImplGlfw_InitForVulkan(mfWindowGetHandle(config->window), true);

        ImGui_ImplVulkan_InitInfo info = {
            .Allocator = backend->ctx.allocator,
            .ApiVersion = VK_API_VERSION_1_2,
            .DescriptorPool = backend->ctx.uiDescPool,
            .ImageCount = FRAMES_IN_FLIGHT,
            .MinImageCount = FRAMES_IN_FLIGHT,
            .Instance = backend->ctx.instance,
            .Device = backend->ctx.device,
            .PhysicalDevice = backend->ctx.physicalDevice,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Subpass = 0,
            .RenderPass = backend->pass,
            .Queue = backend->ctx.qData.gQueue,
            .QueueFamily = backend->ctx.qData.gQueueIdx
        };

        ImGui_ImplVulkan_Init(&info);
        ImGui_ImplVulkan_CreateFontsTexture();
    }
}

void VulkanBackendShutdown(VulkanBackend* backend) {
    if(backend->enableUI) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        igDestroyContext(igGetCurrentContext());
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(backend->ctx.device, backend->imgAvailableSemas[i], backend->ctx.allocator);
        vkDestroySemaphore(backend->ctx.device, backend->rndrFinishedSemas[i], backend->ctx.allocator);
        vkDestroyFence(backend->ctx.device, backend->inFlightFences[i], backend->ctx.allocator);
    }

    for(u32 i = 0; i < backend->fbCount; i++) {
        VulkanFbDestroy(&backend->ctx, backend->fbs[i]);
    }
    
    VulkanRenderPassDestroy(&backend->ctx, backend->pass);
    VulkanBackendCtxDestroy(&backend->ctx);

    MF_FREEMEM(backend->fbs);
    MF_SETMEM(backend, 0, sizeof(VulkanBackend));
}

void VulkanBackendBeginframe(VulkanBackend* backend, MFWindow* window) {
    if(backend->rt != mfnull) {
        VK_CHECK(vkWaitForFences(backend->ctx.device, 1, &backend->rt->fences[backend->frameIndex], VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(backend->ctx.device, 1, &backend->rt->fences[backend->frameIndex]));
    }

    VK_CHECK(vkWaitForFences(backend->ctx.device, 1, &backend->inFlightFences[backend->frameIndex], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(backend->ctx.device, 1, &backend->inFlightFences[backend->frameIndex]));

    VkResult result = vkAcquireNextImageKHR(backend->ctx.device, backend->ctx.swapchain, UINT64_MAX, backend->imgAvailableSemas[backend->frameIndex], VK_NULL_HANDLE, &backend->scImgIdx);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        OnResize(backend, (u32)mfWindowGetConfig(window)->width, (u32)mfWindowGetConfig(window)->height, window);
        if(backend->rt != mfnull) {
            mfRenderTargetResize(backend->rt, (MFVec2){mfRenderTargetGetWidth(backend->rt), mfRenderTargetGetHeight(backend->rt)});
        }
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK(result);
    }

    if(backend->rt != mfnull) {
        mfRenderTargetBegin(backend->rt);
    }
    VK_CHECK(vkResetCommandBuffer(backend->cmdBuffers[backend->frameIndex], 0));
    VulkanCommandBufferBegin(backend->cmdBuffers[backend->frameIndex]);

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

    vkCmdBeginRenderPass(backend->cmdBuffers[backend->frameIndex], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    if(!backend->enableUI)
        return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
}

void VulkanBackendEndframe(VulkanBackend* backend, MFWindow* window) {
    if(backend->rt != mfnull) {
        mfRenderTargetEnd(backend->rt);
    }

    if(backend->enableUI) {
        igEndFrame();
        igRender();
        ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), backend->cmdBuffers[backend->frameIndex], mfnull);

        igUpdatePlatformWindows();
        igRenderPlatformWindowsDefault(mfnull, mfnull);
    }

    vkCmdEndRenderPass(backend->cmdBuffers[backend->frameIndex]);
    VulkanCommandBufferEnd(backend->cmdBuffers[backend->frameIndex]);

    VkPipelineStageFlags waitDstMask[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &backend->cmdBuffers[backend->frameIndex],
        .pWaitDstStageMask = waitDstMask,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &backend->rndrFinishedSemas[backend->frameIndex],
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &backend->imgAvailableSemas[backend->frameIndex]
    };

    if(backend->rt != mfnull) {
        submitInfo.pWaitSemaphores = &backend->rndrFinishedSemas[backend->frameIndex];
    }

    VK_CHECK(vkQueueSubmit(backend->ctx.qData.gQueue, 1, &submitInfo, backend->inFlightFences[backend->frameIndex]));

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &backend->scImgIdx,
        .swapchainCount = 1,
        .pSwapchains = &backend->ctx.swapchain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &backend->rndrFinishedSemas[backend->frameIndex]
    };

    VkResult result = vkQueuePresentKHR(backend->ctx.qData.pQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        OnResize(backend, (u32)mfWindowGetConfig(window)->width, (u32)mfWindowGetConfig(window)->height, window);
        if(backend->rt != mfnull) {
            mfRenderTargetResize(backend->rt, (MFVec2){mfRenderTargetGetWidth(backend->rt), mfRenderTargetGetHeight(backend->rt)});
        }
        return;
    }
    VK_CHECK(result);

    backend->frameIndex = (backend->frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void VulkanBackendDrawVertices(VulkanBackend* backend, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance) {
    VkCommandBuffer buff = backend->cmdBuffers[backend->frameIndex];
    if(backend->rt != mfnull) {
        buff = backend->rt->commandBuffers[backend->frameIndex];
    }

    vkCmdDraw(buff, vertexCount, instances, firstVertex, firstInstance);
}

void VulkanBackendDrawVerticesIndexed(VulkanBackend* backend, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance) {
    VkCommandBuffer buff = backend->cmdBuffers[backend->frameIndex];
    if(backend->rt != mfnull) {
        buff = backend->rt->commandBuffers[backend->frameIndex];
    }

    vkCmdDrawIndexed(buff, indexCount, instances, firstIndex, 0, firstInstance); // NOTE: Make the offset configurable if necessary
}