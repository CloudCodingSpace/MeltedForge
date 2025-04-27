#include "backend.h"

#include "cmd.h"
#include "renderpass.h"
#include "fb.h"

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window) {
    VulkanBckndCtxInit(&backend->ctx, appName, window);

    backend->cmdPool = VulkanCommandPoolCreate(&backend->ctx, backend->ctx.qData.gQueueIdx);
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        backend->cmdBuffers[i] = VulkanCommandBufferAllocate(&backend->ctx, backend->cmdPool, true);
    }

    backend->pass = VulkanRenderPassCreate(&backend->ctx, backend->ctx.scFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    
    // Framebuffers
    backend->fbCount = backend->ctx.scImgCount;
    backend->fbs = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * backend->fbCount);
    for(u32 i = 0; i < backend->fbCount; i++) {
        VkImageView views[] = {
            backend->ctx.scImgViews[i]
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
}

void VulkanBckndShutdown(VulkanBackend* backend) {
    vkDeviceWaitIdle(backend->ctx.device);

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

void VulkanBckndBeginframe(VulkanBackend* backend) {

}

void VulkanBckndEndframe(VulkanBackend* backend) {

}