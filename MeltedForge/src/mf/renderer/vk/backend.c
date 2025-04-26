#include "backend.h"

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window) {
    VulkanBckndCtxInit(&backend->ctx, appName, window);

    backend->cmdPool = VulkanCommandPoolCreate(&backend->ctx, backend->ctx.qData.gQueueIdx);
    backend->cmdBuffer = VulkanCommandBufferAllocate(&backend->ctx, backend->cmdPool, true);
    backend->pass = VulkanRenderPassCreate(&backend->ctx, backend->ctx.scFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void VulkanBckndShutdown(VulkanBackend* backend) {
    VulkanRenderPassDestroy(&backend->ctx, backend->pass);
    VulkanCommandPoolDestroy(&backend->ctx, backend->cmdPool);
    VulkanBckndCtxDestroy(&backend->ctx);

    MF_SETMEM(backend, 0, sizeof(VulkanBackend));
}

void VulkanBckndBeginframe(VulkanBackend* backend) {

}

void VulkanBckndEndframe(VulkanBackend* backend) {

}