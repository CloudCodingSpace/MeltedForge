#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfmaths.h"
#include "renderer/mfutil_types.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
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

typedef struct VulkanBackendCtxConfig_s {
    VkSampleCountFlagBits samples;
    const char* appName;
    bool vsync, headless;
    MFVec2 headlessExtent;
    GLFWwindow* window;
} VulkanBackendCtxConfig;

typedef struct VulkanBackendCtx_s {
    VulkanBackendCtxConfig config;

    VkAllocationCallbacks* allocator;
    VkInstance instance;
    VkSurfaceKHR surface;
#ifdef MF_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; 
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT; 
#endif

    u32 uniqueQueueCount;
    u32 uniqueQueues[4];
    VulkanBackendQueueData queueData;
    MFOptionalRenderFeatures featureFlags;
    VkPhysicalDeviceVulkan12Features features;
    VkPhysicalDeviceVulkan12Properties props;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VmaAllocator vmaAllocator;

    VkExtent2D swapchainExtent;
    VkPresentModeKHR swapchainMode;
    VkSurfaceFormatKHR swapchainFormat;
    VkSwapchainKHR swapchain;
    u32 swapchainImageCount;
    VulkanImage* swapchainImages;
    
    bool hadRenderTargetUsage, renderPassBegun, dispatchBegun;
    VkSampleCountFlagBits maxSupportedSamples, samples;

    VkDescriptorPool uiDescriptorPool;
    VkCommandPool commandPool, computeCommandPool;
} VulkanBackendCtx;

void VulkanBackendCtxInit(VulkanBackendCtx* ctx, VulkanBackendCtxConfig config);
void VulkanBackendCtxDestroy(VulkanBackendCtx* ctx);

void VulkanBackendCtxResize(VulkanBackendCtx* ctx);

#ifdef __cplusplus
}
#endif
