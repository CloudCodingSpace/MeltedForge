#pragma once

#include <vulkan/vulkan.h>

#include "window/mfwindow.h"

typedef struct VulkanBackendQueueData_s {
    i32 gQueueIdx, tQueueIdx, pQueueIdx, cQueueIdx;
    VkQueue gQueue, tQueue, pQueue, cQueue;
} VulkanBackendQueueData;

typedef struct VulkanBackendCtx_s {
    VkAllocationCallbacks* allocator;
    VkInstance instance;
    VkSurfaceKHR surface;

    VulkanBackendQueueData qData;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
} VulkanBackendCtx;

void VulkanBckndCtxInit(VulkanBackendCtx* ctx, const char* appName, MFWindow* window);
void VulkanBckndCtxDestroy(VulkanBackendCtx* ctx);
