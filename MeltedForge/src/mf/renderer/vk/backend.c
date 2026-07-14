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
#include "buffer.h"

void OnResize(VulkanBackend* backend, u32 width, u32 height, MFWindow* window) {
    if(backend->ctx.swapchainExtent.width == width && backend->ctx.swapchainExtent.height == height)
        return;
    
    // Waiting until window res is more than 0
    {
        GLFWwindow* handle = mfWindowGetHandle(window);
        int width, height;
        while(width == 0 || height == 0) {
            glfwGetWindowSize(handle, &width, &height);
            glfwWaitEvents();
        }
    }

    VK_CHECK(vkDeviceWaitIdle(backend->ctx.device));

    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT)
            VulkanImageDestroy(&backend->msaaImages[i]);
        VulkanFramebufferDestroy(&backend->frameBuffers[i]);
    }

    VulkanBackendCtxResize(&backend->ctx, window);

    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        // MSAA Images
        if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT) {
            VulkanImageInfo info = {
                .width = backend->ctx.swapchainExtent.width,
                .height = backend->ctx.swapchainExtent.height,
                .arrayLayers = 1,
                .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .ctx = &backend->ctx,
                .format = backend->ctx.swapchainFormat.format,
                .generateMipmaps = false,
                .gpuResource = false,
                .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                .samples = backend->ctx.samples,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .type = VK_IMAGE_TYPE_2D,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .viewType = VK_IMAGE_VIEW_TYPE_2D
            };
            VulkanImageCreate(&backend->msaaImages[i], info);
        }

        u32 len = 1;
        VulkanImage* attachments[3] = {
            (backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT) ? &backend->msaaImages[i] : &backend->ctx.swapchainImages[i]
        };
        if(backend->config.enableDepth) {
            attachments[len++] = &backend->ctx.depthImage;
        }
        if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT)
            attachments[len++] = &backend->ctx.swapchainImages[i];

        VulkanFramebufferCreate(&backend->frameBuffers[i], &backend->ctx, backend->pass.handle, len, attachments, backend->ctx.swapchainExtent); 
    }

    if(backend->resizeCallback) {
        backend->resizeCallback(backend->callbackState);
    }
}

void VulkanBackendInit(VulkanBackend* backend, VulkanBackendConfig* config) {
    backend->config = *config;
    backend->waitSemas = mfArrayCreate(5, sizeof(VkSemaphore));
    backend->waitStages = mfArrayCreate(5, sizeof(VkPipelineStageFlags));
    backend->descSetBindingPool = mfArrayCreate(5, sizeof(VkDescriptorSet));

    VulkanBackendCtxInit(&backend->ctx, config->msaaSamples, config->appName, config->vsync, config->enableDepth, config->window);

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        backend->commandBuffers[i] = VulkanCommandBufferAllocate(&backend->ctx, backend->ctx.commandPool, true);
        backend->computeCmdBuffers[i] = VulkanCommandBufferAllocate(&backend->ctx, backend->ctx.computeCommandPool, true);
    }

    {
        VulkanRenderPassInfo info = {
            .format = backend->ctx.swapchainFormat.format,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .hasDepth = config->enableDepth,
            .hasMsaa = backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT
        };

        VulkanRenderPassCreate(&backend->pass, &backend->ctx, info);
    }

    // Color images
    if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT) {
        backend->msaaImages = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * backend->ctx.swapchainImageCount);
        for(u32 i = 0; i < backend->ctx.swapchainImageCount; i++) {
            VulkanImageInfo info = {
                .width = backend->ctx.swapchainExtent.width,
                .height = backend->ctx.swapchainExtent.height,
                .arrayLayers = 1,
                .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .ctx = &backend->ctx,
                .format = backend->ctx.swapchainFormat.format,
                .generateMipmaps = false,
                .gpuResource = false,
                .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                .samples = backend->ctx.samples,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .type = VK_IMAGE_TYPE_2D,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .viewType = VK_IMAGE_VIEW_TYPE_2D
            };
            VulkanImageCreate(&backend->msaaImages[i], info);
        }
    }

    // Framebuffers
    backend->frameBufferCount = backend->ctx.swapchainImageCount;
    backend->frameBuffers = MF_ALLOCMEM(VulkanFramebuffer, sizeof(VulkanFramebuffer) * backend->frameBufferCount);
    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        u32 len = 1;
        VulkanImage* attachments[3] = {
            (backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT) ? &backend->msaaImages[i] : &backend->ctx.swapchainImages[i]
        };
        if(config->enableDepth) {
            attachments[len++] = &backend->ctx.depthImage;
        }
        if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT)
            attachments[len++] = &backend->ctx.swapchainImages[i];

        VulkanFramebufferCreate(&backend->frameBuffers[i], &backend->ctx, backend->pass.handle, len, attachments, backend->ctx.swapchainExtent); 
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
            .MSAASamples = backend->ctx.samples,
            .Subpass = 0,
            .RenderPass = backend->pass.handle,
            .Queue = backend->ctx.queueData.graphicsQueue,
            .QueueFamily = backend->ctx.queueData.graphicsQueueIdx
        };

        ImGui_ImplVulkan_Init(&info);
        ImGui_ImplVulkan_CreateFontsTexture();
    }
}

void VulkanBackendShutdown(VulkanBackend* backend) {
    if(backend->config.enableUI) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        igDestroyContext(igGetCurrentContext());
    }

    mfArrayDestroy(&backend->waitSemas);
    mfArrayDestroy(&backend->waitStages);
    mfArrayDestroy(&backend->descSetBindingPool);

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
        VulkanCommandBufferFree(&backend->ctx, backend->commandBuffers[i], backend->ctx.commandPool);
        VulkanCommandBufferFree(&backend->ctx, backend->computeCmdBuffers[i], backend->ctx.computeCommandPool);
    }

    for(u32 i = 0; i < backend->ctx.swapchainImageCount; i++) {
        vkDestroySemaphore(backend->ctx.device, backend->renderFinishedSemas[i], backend->ctx.allocator);
    }

    for(u32 i = 0; i < backend->frameBufferCount; i++) {
        if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT)
            VulkanImageDestroy(&backend->msaaImages[i]);
        VulkanFramebufferDestroy(&backend->frameBuffers[i]);
    }
    
    VulkanRenderPassDestroy(&backend->pass);
    VulkanBackendCtxDestroy(&backend->ctx);

    if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT)
        MF_FREEMEM(backend->msaaImages);
    MF_FREEMEM(backend->renderFinishedSemas);
    MF_FREEMEM(backend->frameBuffers);
    MF_SETMEM(backend, 0, sizeof(VulkanBackend));
}

bool VulkanBackendBeginframe(VulkanBackend* backend, MFWindow* window) {
    // Clearing per frame data
    {
        mfArrayReset(&backend->waitSemas);
        mfArrayReset(&backend->waitStages);
        backend->hadRenderTargetUsage = false;
    }

    VkResult result = vkAcquireNextImageKHR(backend->ctx.device, backend->ctx.swapchain, UINT64_MAX, backend->imageAvailableSemas[backend->frameIndex], VK_NULL_HANDLE, &backend->swapchainImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        OnResize(backend, (u32)mfWindowGetConfig(window)->width, (u32)mfWindowGetConfig(window)->height, window);
        return false;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK(result);
    }

    VK_CHECK(vkResetCommandBuffer(backend->commandBuffers[backend->frameIndex], 0));
    VulkanCommandBufferBegin(backend->commandBuffers[backend->frameIndex], true);

    u32 clearCount = 1;
    VkClearValue values[3] = {
        backend->clearColor
    };
    if(backend->config.enableDepth) {
        values[clearCount].depthStencil.depth = 1.0f;
        values[clearCount++].depthStencil.stencil = 0;
    }
    if(backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT)
        values[clearCount++] = backend->clearColor;
    
    VulkanRenderPassBeginInfo rpInfo = {
        .clearValueCount = clearCount,
        .clearValues = values,
        .fb = &backend->frameBuffers[backend->swapchainImageIndex],
        .extent = (VkRect2D){.extent = backend->ctx.swapchainExtent, .offset = (VkOffset2D){ 0, 0 }},
        .cmdBuff = backend->commandBuffers[backend->frameIndex]
    };

    VulkanRenderPassBegin(&backend->pass, rpInfo);
    backend->renderPassBegun = true;

    if(!backend->config.enableUI)
        return true;

    ImGui_ImplVulkan_NewFrame();    
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();

    return true;
}

void VulkanBackendEndframe(VulkanBackend* backend, MFWindow* window) {
    if(!backend->renderPassBegun)
        return;

    if(backend->config.enableUI) {
        igEndFrame();
        igRender();
        ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), backend->commandBuffers[backend->frameIndex], mfnull);

        igUpdatePlatformWindows();
        igRenderPlatformWindowsDefault(mfnull, mfnull);
    }

    VulkanRenderPassEnd(&backend->pass, backend->commandBuffers[backend->frameIndex], &backend->frameBuffers[backend->swapchainImageIndex]);
    VulkanCommandBufferEnd(backend->commandBuffers[backend->frameIndex]);

    if(!backend->hadRenderTargetUsage) {
        mfArrayAddElement(&backend->waitSemas, VkSemaphore, backend->imageAvailableSemas[backend->frameIndex]);
        mfArrayAddElement(&backend->waitStages, VkPipelineStageFlags, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    VkSemaphore signalSemas[1] = {
        backend->renderFinishedSemas[backend->swapchainImageIndex]
    };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &backend->commandBuffers[backend->frameIndex],
        .pWaitDstStageMask = (VkPipelineStageFlags*)backend->waitStages.data,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemas,
        .waitSemaphoreCount = backend->waitSemas.len,
        .pWaitSemaphores = (VkSemaphore*)backend->waitSemas.data
    };

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
    backend->renderPassBegun = false;
    backend->renderTarget = mfnull;
}

void VulkanBackendWaitForFrame(VulkanBackend* backend) {
    VK_CHECK(vkWaitForFences(backend->ctx.device, 1, &backend->inFlightFences[backend->frameIndex], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(backend->ctx.device, 1, &backend->inFlightFences[backend->frameIndex]));
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

void VulkanBackendSetCurrentImagePixels(VulkanBackend* backend, u8* pixels) {
    if(backend->renderPassBegun)
        return;

    VulkanImageSetPixels(&backend->ctx.swapchainImages[backend->swapchainImageIndex], pixels);
}

u8* VulkanBackendGetCurrentImagePixels(VulkanBackend* backend, u32* width, u32* height) {
    if(backend->renderPassBegun)
        return mfnull;
    
    return VulkanImageGetPixels(&backend->ctx.swapchainImages[backend->swapchainImageIndex], 0, 0, width, height);
}

#ifdef __cplusplus
}
#endif