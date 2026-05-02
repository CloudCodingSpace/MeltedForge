#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "window/mfwindow.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

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
    VmaAllocator vmaAllocator;

    VkExtent2D swapchainExtent;
    VkPresentModeKHR swapchainMode;
    VkSurfaceFormatKHR swapchainFormat;
    VkSwapchainKHR swapchain;
    u32 swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    
    bool vsync, enableDepth;

    VkFormat depthFormat;
    VulkanImage depthImage;

    VkDescriptorPool uiDescriptorPool;
    VkCommandPool commandPool;
} VulkanBackendCtx;

void VulkanBackendCtxInit(VulkanBackendCtx* ctx, const char* appName, bool vsync, bool enableDepth, MFWindow* window);
void VulkanBackendCtxDestroy(VulkanBackendCtx* ctx);

void VulkanBackendCtxResize(VulkanBackendCtx* ctx, MFWindow* window);

#ifdef __cplusplus
}
#endif