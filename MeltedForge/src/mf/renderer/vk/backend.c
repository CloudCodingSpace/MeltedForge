#include "backend.h"

#include <GLFW/glfw3.h>

void VulkanBckndInit(VulkanBackend* backend, const char* appName, MFWindow* window) {
    VulkanBckndCtxInit(&backend->ctx, appName, window);
}

void VulkanBckndShutdown(VulkanBackend* backend) {
    VulkanBckndCtxDestroy(&backend->ctx);
}

void VulkanBckndBeginframe(VulkanBackend* backend) {

}

void VulkanBckndEndframe(VulkanBackend* backend) {

}