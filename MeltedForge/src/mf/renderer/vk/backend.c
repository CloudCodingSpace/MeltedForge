#ifdef __cplusplus
extern "C" {
#endif

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
    if(backend->ctx.swapchainExtent.width == width && backend->ctx.swapchainExtent.height == height)
        return;
    if(width == 0 && height == 0)
        return;

    VK_CHECK(vkDeviceWaitIdle(backend->ctx.device));

    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        VulkanFbDestroy(&backend->ctx, backend->frameBuffers[i]);
    }

    VulkanBackendCtxResize(&backend->ctx, window);

    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        u32 len = 1;
        VkImageView views[2] = {
            backend->ctx.swapchainImageViews[i]
        };
        if(backend->enableDepth) {
            len++;
            views[1] = backend->ctx.depthImage.view;
        }

        backend->frameBuffers[i] = VulkanFbCreate(&backend->ctx, backend->pass, len, views, backend->ctx.swapchainExtent); 
    }

    if(backend->resizeCallback) {
        backend->resizeCallback(backend->callbackState);
    }

    if(backend->renderTarget != mfnull) {
        mfRenderTargetResize(backend->renderTarget, (MFVec2){mfRenderTargetGetWidth(backend->renderTarget), mfRenderTargetGetHeight(backend->renderTarget)});
    }
}

void VulkanBackendInit(VulkanBackend* backend, VulkanBackendConfig* config) {
    backend->enableUI = config->enableUI;
    backend->enableDepth = config->enableDepth;

    VulkanBackendCtxInit(&backend->ctx, config->appName, config->vsync, config->enableDepth, config->window);

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        backend->commandBuffers[i] = VulkanCommandBufferAllocate(&backend->ctx, backend->ctx.commandPool, true);
    }

    {
        VulkanRenderPassInfo info = {
            .format = backend->ctx.swapchainFormat.format,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .hasDepth = config->enableDepth,
            .renderTarget = false
        };

        backend->pass = VulkanRenderPassCreate(&backend->ctx, info);
    }
    
    // Framebuffers
    backend->frameBufferCount = backend->ctx.swapchainImageCount;
    backend->frameBuffers = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * backend->frameBufferCount);
    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        u32 len = 1;
        VkImageView views[2] = {
            backend->ctx.swapchainImageViews[i]
        };
        if(config->enableDepth) {
            len++;
            views[1] = backend->ctx.depthImage.view;
        }

        backend->frameBuffers[i] = VulkanFbCreate(&backend->ctx, backend->pass, len, views, backend->ctx.swapchainExtent); 
    }
    
    // Sync objs
    {
        VkSemaphoreCreateInfo semaInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VK_CHECK(vkCreateSemaphore(backend->ctx.device, &semaInfo, backend->ctx.allocator, &backend->imageAvailableSemas[i]));
            VK_CHECK(vkCreateFence(backend->ctx.device, &fenceInfo, backend->ctx.allocator, &backend->inFlightFences[i]));
        }
        backend->renderFinishedSemas = MF_ALLOCMEM(VkSemaphore, sizeof(VkSemaphore) * backend->ctx.swapchainImageCount);
        for(u32 i = 0; i < backend->ctx.swapchainImageCount; i++) {
            VK_CHECK(vkCreateSemaphore(backend->ctx.device, &semaInfo, backend->ctx.allocator, &backend->renderFinishedSemas[i]));
        }
    }

    // Pipeline cache
    {
        backend->pipelineCacheFilePath = mfStringDuplicate("mfpipeline_caches.bin");
        u8* initialData = mfnull;
        VkPipelineCacheCreateInfo cacheInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .initialDataSize = 0,
            .pInitialData = mfnull
        };

        size_t size = 0;
        bool success = false;
        initialData = mfReadFile(mfGetLogger(), &size, &success, backend->pipelineCacheFilePath, "rb");
        if(success) {
            cacheInfo.initialDataSize = size;
            cacheInfo.pInitialData = initialData;
        }

        VkResult cacheResult = vkCreatePipelineCache(backend->ctx.device, &cacheInfo, backend->ctx.allocator, &backend->pipelineCache);
        if(cacheResult != VK_SUCCESS) {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to create the pipeline cache! Result by vulkan :- %s", string_VkResult(cacheResult));
            backend->pipelineCache = mfnull;
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
            .DescriptorPool = backend->ctx.uiDescriptorPool,
            .ImageCount = FRAMES_IN_FLIGHT,
            .MinImageCount = FRAMES_IN_FLIGHT,
            .Instance = backend->ctx.instance,
            .Device = backend->ctx.device,
            .PhysicalDevice = backend->ctx.physicalDevice,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Subpass = 0,
            .RenderPass = backend->pass,
            .Queue = backend->ctx.queueData.graphicsQueue,
            .QueueFamily = backend->ctx.queueData.graphicsQueueIdx
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

    if(backend->pipelineCache) {
        size_t size = 0;
        VkResult result = vkGetPipelineCacheData(backend->ctx.device, backend->pipelineCache, &size, mfnull);
        if(result == VK_SUCCESS) {
            u8* buffer = MF_ALLOCMEM(u8, sizeof(u8) * size);
            result = vkGetPipelineCacheData(backend->ctx.device, backend->pipelineCache, &size, buffer);
            if(result != VK_SUCCESS)
                slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to get the pipeline cache's data! Result by vulkan :- %s", string_VkResult(result));
            else
                mfWriteFile(mfGetLogger(), size, backend->pipelineCacheFilePath, buffer, "wb");

            MF_FREEMEM(buffer);
        } else {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to get the pipeline cache's data! Result by vulkan :- %s", string_VkResult(result));
        }

        MF_FREEMEM(backend->pipelineCacheFilePath);
        vkDestroyPipelineCache(backend->ctx.device, backend->pipelineCache, backend->ctx.allocator);
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(backend->ctx.device, backend->imageAvailableSemas[i], backend->ctx.allocator);
        vkDestroyFence(backend->ctx.device, backend->inFlightFences[i], backend->ctx.allocator);
    }

    for(u32 i = 0; i < backend->ctx.swapchainImageCount; i++) {
        vkDestroySemaphore(backend->ctx.device, backend->renderFinishedSemas[i], backend->ctx.allocator);
    }

    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        VulkanFbDestroy(&backend->ctx, backend->frameBuffers[i]);
    }
    
    VulkanRenderPassDestroy(&backend->ctx, backend->pass);
    VulkanBackendCtxDestroy(&backend->ctx);

    MF_FREEMEM(backend->renderFinishedSemas);
    MF_FREEMEM(backend->frameBuffers);
    MF_SETMEM(backend, 0, sizeof(VulkanBackend));
}

void VulkanBackendBeginframe(VulkanBackend* backend, MFWindow* window) {
    VK_CHECK(vkWaitForFences(backend->ctx.device, 1, &backend->inFlightFences[backend->frameIndex], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(backend->ctx.device, 1, &backend->inFlightFences[backend->frameIndex]));

    VkResult result = vkAcquireNextImageKHR(backend->ctx.device, backend->ctx.swapchain, UINT64_MAX, backend->imageAvailableSemas[backend->frameIndex], VK_NULL_HANDLE, &backend->swapchainImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        OnResize(backend, (u32)mfWindowGetConfig(window)->width, (u32)mfWindowGetConfig(window)->height, window);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK(result);
    }

    if(backend->renderTarget != mfnull) {
        mfRenderTargetBegin(backend->renderTarget);
    }
    VK_CHECK(vkResetCommandBuffer(backend->commandBuffers[backend->frameIndex], 0));
    VulkanCommandBufferBegin(backend->commandBuffers[backend->frameIndex]);

    u32 clearCount = 1;
    VkClearValue values[2] = {
        backend->clearColor
    };
    if(backend->enableDepth) {
        clearCount++;
        values[1].depthStencil.depth = 1.0f;
        values[1].depthStencil.stencil = 0;
    }
    VkRenderPassBeginInfo rpInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = clearCount,
        .pClearValues = values,
        .framebuffer = backend->frameBuffers[backend->swapchainImageIndex],
        .renderArea = (VkRect2D){.extent = backend->ctx.swapchainExtent, .offset = (VkOffset2D){ 0, 0 }},
        .renderPass = backend->pass
    };

    vkCmdBeginRenderPass(backend->commandBuffers[backend->frameIndex], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    if(!backend->enableUI)
        return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
}

void VulkanBackendEndframe(VulkanBackend* backend, MFWindow* window) {
    if(backend->renderTarget != mfnull) {
        mfRenderTargetEnd(backend->renderTarget);
    }

    if(backend->enableUI) {
        igEndFrame();
        igRender();
        ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), backend->commandBuffers[backend->frameIndex], mfnull);

        igUpdatePlatformWindows();
        igRenderPlatformWindowsDefault(mfnull, mfnull);
    }

    vkCmdEndRenderPass(backend->commandBuffers[backend->frameIndex]);
    VulkanCommandBufferEnd(backend->commandBuffers[backend->frameIndex]);

    VkPipelineStageFlags waitDstMask[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore waitSemas[1] = {
        backend->imageAvailableSemas[backend->frameIndex]
    };

    VkSemaphore signalSemas[1] = {
        backend->renderFinishedSemas[backend->swapchainImageIndex]
    };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &backend->commandBuffers[backend->frameIndex],
        .pWaitDstStageMask = waitDstMask,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemas,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemas
    };

    if(backend->renderTarget != mfnull) {
        waitSemas[0] = backend->renderTarget->renderFinishedSemas[backend->swapchainImageIndex];
    }

    VK_CHECK(vkQueueSubmit(backend->ctx.queueData.graphicsQueue, 1, &submitInfo, backend->inFlightFences[backend->frameIndex]));

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &backend->swapchainImageIndex,
        .swapchainCount = 1,
        .pSwapchains = &backend->ctx.swapchain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemas
    };

    VkResult result = vkQueuePresentKHR(backend->ctx.queueData.presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        OnResize(backend, (u32)mfWindowGetConfig(window)->width, (u32)mfWindowGetConfig(window)->height, window);
        return;
    }
    VK_CHECK(result);

    backend->frameIndex = (backend->frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void VulkanBackendDrawVertices(VulkanBackend* backend, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance) {
    VkCommandBuffer buff = backend->commandBuffers[backend->frameIndex];
    if(backend->renderTarget != mfnull) {
        buff = backend->renderTarget->commandBuffers[backend->frameIndex];
    }

    vkCmdDraw(buff, vertexCount, instances, firstVertex, firstInstance);
}

void VulkanBackendDrawVerticesIndexed(VulkanBackend* backend, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance) {
    VkCommandBuffer buff = backend->commandBuffers[backend->frameIndex];
    if(backend->renderTarget != mfnull) {
        buff = backend->renderTarget->commandBuffers[backend->frameIndex];
    }

    vkCmdDrawIndexed(buff, indexCount, instances, firstIndex, 0, firstInstance); // NOTE: Make the offset configurable if necessary
}

#ifdef __cplusplus
}
#endif