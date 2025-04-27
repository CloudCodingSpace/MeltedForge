#pragma once

#include <vulkan/vulkan.h>

#include "window/mfwindow.h"

#include "image.h"

typedef struct VulkanBackendQueueData_s {
    i32 gQueueIdx, tQueueIdx, pQueueIdx, cQueueIdx;
    VkQueue gQueue, tQueue, pQueue, cQueue;
} VulkanBackendQueueData;

typedef struct VulkanScCaps_s {
    VkSurfaceCapabilitiesKHR caps;
    u32 modeCount, formatCount;
	VkPresentModeKHR* modes;
	VkSurfaceFormatKHR* formats;
} VulkanScCaps;

typedef struct VulkanBackendCtx_s {
    VkAllocationCallbacks* allocator;
    VkInstance instance;
    VkSurfaceKHR surface;

    VulkanBackendQueueData qData;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkExtent2D scExtent;
    VkPresentModeKHR scMode;
    VkSurfaceFormatKHR scFormat;
    VkSwapchainKHR swapchain;
    u32 scImgCount;
    VkImage* scImgs;
    VkImageView* scImgViews;

    VkFormat depthFormat;
    VulkanImage depthImage;
} VulkanBackendCtx;

void VulkanBckndCtxInit(VulkanBackendCtx* ctx, const char* appName, MFWindow* window);
void VulkanBckndCtxDestroy(VulkanBackendCtx* ctx);
