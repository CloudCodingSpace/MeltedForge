#include "ctx.h"

#include <GLFW/glfw3.h>

#include <string.h>

#include "common.h"

static VulkanBackendQueueData GetDeviceQueueData(VkSurfaceKHR surface, VkPhysicalDevice device) {
    VulkanBackendQueueData data = {-1};

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

static b8 IsQueueDataComplete(VulkanBackendQueueData data) {
    return data.cQueueIdx != -1 && data.gQueueIdx != -1 && data.pQueueIdx != -1 && data.tQueueIdx != -1;
}

static b8 IsDeviceUsable(VkSurfaceKHR surface, VkPhysicalDevice device) {
    VulkanBackendQueueData data = GetDeviceQueueData(surface, device);

    b8 extSupport = false;
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

static VulkanScCaps GetScCaps(VulkanBackendCtx* ctx) {
    VulkanScCaps caps = {0};
    
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &caps.caps));

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &caps.modeCount, mfnull));
    caps.modes = MF_ALLOCMEM(VkPresentModeKHR, sizeof(VkPresentModeKHR) * caps.modeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &caps.modeCount, caps.modes));

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &caps.formatCount, mfnull));
    caps.formats = MF_ALLOCMEM(VkSurfaceFormatKHR, sizeof(VkSurfaceFormatKHR) * caps.formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &caps.formatCount, caps.formats));
    
    return caps;
}

static void SelectScCaps(VulkanBackendCtx* ctx, VulkanScCaps caps, GLFWwindow* window) {

}

static void CreateSwapchain(VulkanBackendCtx* ctx, GLFWwindow* window) {
    VulkanScCaps caps = GetScCaps(ctx);

    // Selecting the present mode
	{
		bool set = false;
		for (u32 i = 0; i < caps.modeCount; i++) {
			if (caps.modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				ctx->scMode = caps.modes[i];
				set = true;
				break;
			}
		}

		if (!set)
			ctx->scMode = VK_PRESENT_MODE_FIFO_KHR;
	}
	// Selecting the surface format
	{
		bool set = false;
		for (u32 i = 0; i < caps.formatCount; i++) {
			if (caps.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && caps.formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				ctx->scFormat = caps.formats[i];
				set = true;
				break;
			}
		}
		if (!set)
			ctx->scFormat = caps.formats[0];
	}
	// Selecting the extent
	{
		if (caps.caps.currentExtent.width != UINT32_MAX)
			ctx->scExtent = caps.caps.currentExtent;
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			ctx->scExtent = (VkExtent2D) {
				(u32)width,
				(u32)height
			};

			ctx->scExtent.width = MF_CLAMP(ctx->scExtent.width, caps.caps.minImageExtent.width, caps.caps.maxImageExtent.width);
			ctx->scExtent.height = MF_CLAMP(ctx->scExtent.height, caps.caps.minImageExtent.height, caps.caps.maxImageExtent.height);
		}
	}
    // Creating the swapchain
    {
        VkSurfaceCapabilitiesKHR surfaceCaps = caps.caps;

        uint32_t imgCount = surfaceCaps.minImageCount + 1;
		if (surfaceCaps.maxImageCount > 0 && imgCount > surfaceCaps.maxImageCount) {
			imgCount = surfaceCaps.maxImageCount;
		}

		VkSwapchainCreateInfoKHR info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    		.surface = ctx->surface,
    		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    		.minImageCount = imgCount,
    		.imageFormat = ctx->scFormat.format,
    		.imageColorSpace = ctx->scFormat.colorSpace,
    		.preTransform = surfaceCaps.currentTransform,
    		.presentMode = ctx->scMode,
    		.imageArrayLayers = 1,
    		.imageExtent = ctx->scExtent,
    		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    		.clipped = VK_TRUE,
    		.oldSwapchain = mfnull
        };

		u32 queueCount = 0;
        u32 queues[4];
        MF_SETMEM(queues, 0, sizeof(u32) * 4);

        u32 qs[] = {
            ctx->qData.cQueueIdx,
            ctx->qData.pQueueIdx,
            ctx->qData.gQueueIdx,
            ctx->qData.tQueueIdx
        };
        
        // Checking for duplicate queues
        {
            for(u32 i = 0; i < 4; i++) {
                b8 isUnique = true;
    
                for(u32 j = 0; j < queueCount; j++) {
                    if(qs[i] == queues[j]) {
                        isUnique = false;
                        break;
                    }
                }
    
                if(isUnique) {
                    queues[queueCount++] = qs[i];
                }
            }
        }

		if (queueCount > 1)
		{
			info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			info.queueFamilyIndexCount = 4;
			info.pQueueFamilyIndices = qs;
		}
		else
		{
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		VK_CHECK(vkCreateSwapchainKHR(ctx->device, &info, ctx->allocator, &ctx->swapchain));
    }
    // Getting the swapchain images
    {
        VK_CHECK(vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->scImgCount, mfnull));
        ctx->scImgs = MF_ALLOCMEM(VkImage, sizeof(VkImage) * ctx->scImgCount);
        VK_CHECK(vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->scImgCount, ctx->scImgs));
    }
    // Creating the image views
    {
        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format = ctx->scFormat.format,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseArrayLayer = 0,
                .baseMipLevel = 0,
                .layerCount = 1,
                .levelCount = 1
            },
        };

        ctx->scImgViews = MF_ALLOCMEM(VkImageView, sizeof(VkImageView) * ctx->scImgCount);
        for(u32 i = 0; i < ctx->scImgCount; i++) {
            info.image = ctx->scImgs[i];

            VK_CHECK(vkCreateImageView(ctx->device, &info, ctx->allocator, &ctx->scImgViews[i]));
        }
    }

    MF_FREEMEM(caps.formats);
    MF_FREEMEM(caps.modes);
}

void GetDepthFormat(VulkanBackendCtx* ctx) {
    ctx->depthFormat = VK_FORMAT_UNDEFINED;

    VkFormat reqFormats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for(u32 i = 0; i < MF_ARRAYLEN(reqFormats, VkFormat); i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx->physicalDevice, reqFormats[i], &props);

        if((props.linearTilingFeatures & flags) == flags) {
            ctx->depthFormat = reqFormats[i];
            return;
        }
        if((props.optimalTilingFeatures & flags) == flags) {
            ctx->depthFormat = reqFormats[i];
            return;
        }
    }

    MF_FATAL_ABORT(mfGetLogger(), "(From the vulkan backend) Failed to find suitable depth format!");
}

void VulkanBckndCtxInit(VulkanBackendCtx* ctx, const char* appName, MFWindow* window) {
    ctx->allocator = mfnull; // TODO: Create a custom allocator

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

        VK_CHECK(vkCreateInstance(&info, ctx->allocator, &ctx->instance));
    }
    // Surface
    {
        VK_CHECK(glfwCreateWindowSurface(ctx->instance, mfGetWindowHandle(window), ctx->allocator, &ctx->surface));
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
    // Device 
    {
        u32 deviceCount = 1;
        const char* deviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        u32 queueCount = 0;
        u32 queues[4];
        MF_SETMEM(queues, 0, sizeof(u32) * 4);
        // Checking for duplicate queues
        {
            u32 qs[] = {
                ctx->qData.cQueueIdx,
                ctx->qData.pQueueIdx,
                ctx->qData.gQueueIdx,
                ctx->qData.tQueueIdx
            };

            for(u32 i = 0; i < 4; i++) {
                b8 isUnique = true;
    
                for(u32 j = 0; j < queueCount; j++) {
                    if(qs[i] == queues[j]) {
                        isUnique = false;
                        break;
                    }
                }
    
                if(isUnique) {
                    queues[queueCount++] = qs[i];
                }
            }
        }

        float qPriority = 1.0f;

        VkDeviceQueueCreateInfo qInfos[queueCount];
        MF_SETMEM(qInfos, 0, sizeof(VkDeviceQueueCreateInfo) * queueCount);
        for(u32 i = 0; i < queueCount; i++) {
            qInfos[i] = (VkDeviceQueueCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pQueuePriorities = &qPriority,
                .queueCount = 1,
                .queueFamilyIndex = queues[i]
            };
        }

        VkPhysicalDeviceFeatures features = {0};
        vkGetPhysicalDeviceFeatures(ctx->physicalDevice, &features);

        VkDeviceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .enabledExtensionCount = deviceCount,
            .ppEnabledExtensionNames = deviceExts,
            .pEnabledFeatures = &features,
            .queueCreateInfoCount = queueCount,
            .pQueueCreateInfos = qInfos
        };

        VK_CHECK(vkCreateDevice(ctx->physicalDevice, &info, ctx->allocator, &ctx->device));
    }
    // Queues 
    {
        vkGetDeviceQueue(ctx->device, (u32)ctx->qData.cQueueIdx, 0, &ctx->qData.cQueue);
        vkGetDeviceQueue(ctx->device, (u32)ctx->qData.tQueueIdx, 0, &ctx->qData.tQueue);
        vkGetDeviceQueue(ctx->device, (u32)ctx->qData.pQueueIdx, 0, &ctx->qData.pQueue);
        vkGetDeviceQueue(ctx->device, (u32)ctx->qData.gQueueIdx, 0, &ctx->qData.gQueue);
    }
    // Swapchain
    CreateSwapchain(ctx, mfGetWindowHandle(window));
    // Depth
    {
        GetDepthFormat(ctx);

        VulkanImageCreate(&ctx->depthImage, ctx, ctx->scExtent.width, ctx->scExtent.height, ctx->depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}

void VulkanBckndCtxDestroy(VulkanBackendCtx* ctx) {
    VulkanImageDestroy(&ctx->depthImage, ctx);

    for(u32 i = 0; i < ctx->scImgCount; i++)
        vkDestroyImageView(ctx->device, ctx->scImgViews[i], ctx->allocator);
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, ctx->allocator);

    vkDestroyDevice(ctx->device, ctx->allocator);

    vkDestroySurfaceKHR(ctx->instance, ctx->surface, ctx->allocator);
    vkDestroyInstance(ctx->instance, ctx->allocator);

    MF_FREEMEM(ctx->scImgViews);
    MF_FREEMEM(ctx->scImgs);
    MF_SETMEM(ctx, 0, sizeof(VulkanBackendCtx));
}
