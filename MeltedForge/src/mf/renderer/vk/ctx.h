#pragma once

#include <vulkan/vulkan.h>

#include "window/mfwindow.h"

#include "image.h"

typedef struct VulkanBackendQueueData_s {
    i32 graphicsQueueIdx, transferQueueIdx, presentQueueIdx, computeQueueIdx;
    VkQueue graphicsQueue, transferQueue, presentQueue, computeQueue;
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
#ifdef MF_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; 
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT; 
#endif

    VulkanBackendQueueData queueData;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkExtent2D swapchainExtent;
    VkPresentModeKHR swapchainMode;
    VkSurfaceFormatKHR swapchainFormat;
    VkSwapchainKHR swapchain;
    u32 swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    
    b8 vsync, enableDepth;

    VkFormat depthFormat;
    VulkanImage depthImage;

    VkDescriptorPool uiDescriptorPool;
    VkCommandPool commandPool;
} VulkanBackendCtx;

void VulkanBackendCtxInit(VulkanBackendCtx* ctx, const char* appName, b8 vsync, b8 enableDepth, MFWindow* window);
void VulkanBackendCtxDestroy(VulkanBackendCtx* ctx);

void VulkanBackendCtxResize(VulkanBackendCtx* ctx, u32 width, u32 height, MFWindow* window);