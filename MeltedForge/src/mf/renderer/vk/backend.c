#include "backend.h"

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window) {
    VulkanBckndCtxInit(&backend->ctx, appName, window);

    backend->cmdPool = VulkanCommandPoolCreate(&backend->ctx, backend->ctx.qData.gQueueIdx);
    backend->cmdBuffer = VulkanCommandBufferAllocate(&backend->ctx, backend->cmdPool, true);
}

void VulkanBckndShutdown(VulkanBackend* backend) {
    VulkanCommandPoolDestroy(&backend->ctx, backend->cmdPool);
    VulkanBckndCtxDestroy(&backend->ctx);

    MF_SETMEM(backend, 0, sizeof(VulkanBackend));
}

void VulkanBckndBeginframe(VulkanBackend* backend) {

}

void VulkanBckndEndframe(VulkanBackend* backend) {

}