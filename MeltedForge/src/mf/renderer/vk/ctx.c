#include "ctx.h"

#include <GLFW/glfw3.h>

#include <string.h>

static MFVkBackendQueueData GetDeviceQueueData(VkSurfaceKHR surface, VkPhysicalDevice device) {
    MFVkBackendQueueData data = {-1};

    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, mfnull);
    VkQueueFamilyProperties props[count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);

    for(u32 i = 0; i < count; i++) {
        if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            data.gQueueIdx = i;
        if(props[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            data.tQueueIdx = i;
        if(props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            data.cQueueIdx = i;
            
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport)
                data.pQueueIdx = i;
    }

    return data;
}

static bool IsQueueDataComplete(MFVkBackendQueueData data) {
    return data.cQueueIdx != -1 && data.gQueueIdx != -1 && data.pQueueIdx != -1 && data.tQueueIdx != -1;
}

static bool IsDeviceUsable(VkSurfaceKHR surface, VkPhysicalDevice device) {
    MFVkBackendQueueData data = GetDeviceQueueData(surface, device);

    bool extSupport = false;
    {
        const char* deviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        u32 deviceExtCount = 1;

        u32 count = 0;
        vkEnumerateDeviceExtensionProperties(device, mfnull, &count, mfnull);
        VkExtensionProperties props[count];
        vkEnumerateDeviceExtensionProperties(device, mfnull, &count, props);

        for(u32 i = 0; i < count; i++) {
            for(u32 j = 0; j < deviceExtCount; j++) {
                if(strcmp(props[i].extensionName, deviceExts[j]) == 0) {
                    extSupport = true;
                    break;
                }
            }
        }
    }

    return IsQueueDataComplete(data) && extSupport;
}

void mfVkBckndCtxInit(MFVkBackendCtx* ctx, const char* appName, MFWindow* window) {
    ctx->callbacks = mfnull; // TODO: Create a custom allocator

    // Vulkan instance
    {
        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pApplicationName = appName,
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "MeltedForge",
            .apiVersion = VK_API_VERSION_1_0
        };

        u32 extCount = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = extCount,
            .ppEnabledExtensionNames = exts
        };

        VK_CHECK(vkCreateInstance(&info, ctx->callbacks, &ctx->instance));
    }
    // Surface
    {
        VK_CHECK(glfwCreateWindowSurface(ctx->instance, mfGetWindowHandle(window), ctx->callbacks, &ctx->surface));
    }
    // Physical Device 
    {
        u32 deviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, mfnull));
        VkPhysicalDevice devices[deviceCount];
        VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices));

        // TODO: Select the most capable physical device if there are more than one
        for(int i = 0; i < deviceCount; i++) {
            if(IsDeviceUsable(ctx->surface, devices[i])) {
                ctx->physicalDevice = devices[i];
                break;
            }
        }

        MF_ASSERT(ctx->physicalDevice == mfnull, mfGetLogger(), "(From the vulkan backend) Failed to select a suitable GPU in the current PC!");

        ctx->qData = GetDeviceQueueData(ctx->surface, ctx->physicalDevice);
    }
}

void mfVkBckndCtxDestroy(MFVkBackendCtx* ctx) {
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, ctx->callbacks);
    vkDestroyInstance(ctx->instance, ctx->callbacks);

    ctx->callbacks = 0;
    ctx->instance = 0;
}
