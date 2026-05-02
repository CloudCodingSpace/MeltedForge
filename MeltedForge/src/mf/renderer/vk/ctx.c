#ifdef __cplusplus
extern "C" {
#endif

#include "ctx.h"

#include <GLFW/glfw3.h>

#include <string.h>
#include <vulkan/vulkan_core.h>

#include "common.h"
#include "command_buffer.h"

static VKAPI_PTR VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData) {
    SLogger* logger = pUserData;
    SLSeverity severity;
    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severity = SLOG_SEVERITY_WARN;
    } else {
        severity = SLOG_SEVERITY_ERROR;
    }
    slogLogMsg(logger, severity, "(From the vulkan backend) %s", pCallbackData->pMessage);
}

static VulkanBackendQueueData GetDeviceQueueData(VkSurfaceKHR surface, VkPhysicalDevice device) {
    VulkanBackendQueueData data = {-1};

    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, mfnull);
    VkQueueFamilyProperties* props = MF_ALLOCMEM(VkQueueFamilyProperties, sizeof(VkQueueFamilyProperties) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);

    for(u32 i = 0; i < count; i++) {
        if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            data.graphicsQueueIdx = i;
        if(props[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            data.transferQueueIdx = i;
        if(props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            data.computeQueueIdx = i;
            
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport)
                data.presentQueueIdx = i;
    }

    MF_FREEMEM(props);
    return data;
}

static bool IsQueueDataComplete(VulkanBackendQueueData data) {
    return data.computeQueueIdx != -1 && data.graphicsQueueIdx != -1 && data.presentQueueIdx != -1 && data.transferQueueIdx != -1;
}

static bool IsDeviceUsable(VkSurfaceKHR surface, VkPhysicalDevice device) {
    VulkanBackendQueueData data = GetDeviceQueueData(surface, device);

    bool extSupport = false;
    {
        const char* deviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        u32 deviceExtCount = 1;

        u32 count = 0;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(device, mfnull, &count, mfnull));
        VkExtensionProperties* props = MF_ALLOCMEM(VkExtensionProperties, sizeof(VkExtensionProperties) * count);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(device, mfnull, &count, props));

        for(u32 i = 0; i < count; i++) {
            for(u32 j = 0; j < deviceExtCount; j++) {
                if(strcmp(props[i].extensionName, deviceExts[j]) == 0) {
                    extSupport = true;
                    MF_FREEMEM(props);
                    break;
                }
            }

            if(props == mfnull)
                break;
        }
        if(props != mfnull)
            MF_FREEMEM(props);
    }

    VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES
    };

    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &scalarFeatures
    };
    vkGetPhysicalDeviceFeatures2(device, &features);

    return IsQueueDataComplete(data) && extSupport && scalarFeatures.scalarBlockLayout;
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
    // Selecting the present mode
	{
		bool set = false;
		for (u32 i = 0; i < caps.modeCount; i++) {
			if (caps.modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				ctx->swapchainMode = caps.modes[i];
				set = true;
				break;
			}
		}

		if (!set) {
            if(ctx->vsync) {
			    ctx->swapchainMode = VK_PRESENT_MODE_FIFO_KHR;
            }
            else {
			    ctx->swapchainMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }
	// Selecting the surface format
	{
		bool set = false;
		for (u32 i = 0; i < caps.formatCount; i++) {
			if (caps.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && caps.formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				ctx->swapchainFormat = caps.formats[i];
				set = true;
				break;
			}
		}
		if (!set)
			ctx->swapchainFormat = caps.formats[0];
	}
	// Selecting the extent
	{
		if (caps.caps.currentExtent.width != UINT32_MAX)
			ctx->swapchainExtent = caps.caps.currentExtent;
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			ctx->swapchainExtent = (VkExtent2D) {
				(u32)width,
				(u32)height
			};

			ctx->swapchainExtent.width = MF_CLAMP(ctx->swapchainExtent.width, caps.caps.minImageExtent.width, caps.caps.maxImageExtent.width);
			ctx->swapchainExtent.height = MF_CLAMP(ctx->swapchainExtent.height, caps.caps.minImageExtent.height, caps.caps.maxImageExtent.height);
		}
	}
}

static void CreateSwapchain(VulkanBackendCtx* ctx, GLFWwindow* window) {
    VulkanScCaps caps = GetScCaps(ctx);

    SelectScCaps(ctx, caps, window);
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
    		.imageFormat = ctx->swapchainFormat.format,
    		.imageColorSpace = ctx->swapchainFormat.colorSpace,
    		.preTransform = surfaceCaps.currentTransform,
    		.presentMode = ctx->swapchainMode,
    		.imageArrayLayers = 1,
    		.imageExtent = ctx->swapchainExtent,
    		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    		.clipped = VK_TRUE,
    		.oldSwapchain = mfnull
        };

		u32 queueCount = 0;
        u32 queues[4];
        MF_SETMEM(queues, 0, sizeof(u32) * 4);

        u32 qs[] = {
            ctx->queueData.computeQueueIdx,
            ctx->queueData.presentQueueIdx,
            ctx->queueData.graphicsQueueIdx,
            ctx->queueData.transferQueueIdx
        };
        
        // Checking for duplicate queues
        {
            for(u32 i = 0; i < 4; i++) {
                bool isUnique = true;
    
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
        VK_CHECK(vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, mfnull));
        ctx->swapchainImages = MF_ALLOCMEM(VkImage, sizeof(VkImage) * ctx->swapchainImageCount);
        VK_CHECK(vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, ctx->swapchainImages));
    }
    // Creating the image views
    {
        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format = ctx->swapchainFormat.format,
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

        ctx->swapchainImageViews = MF_ALLOCMEM(VkImageView, sizeof(VkImageView) * ctx->swapchainImageCount);
        for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
            info.image = ctx->swapchainImages[i];

            VK_CHECK(vkCreateImageView(ctx->device, &info, ctx->allocator, &ctx->swapchainImageViews[i]));
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

void VulkanBackendCtxInit(VulkanBackendCtx* ctx, const char* appName, bool vsync, bool enableDepth, MFWindow* window) {
    ctx->allocator = mfnull; // TODO: Create a custom allocator
    ctx->vsync = vsync;
    ctx->enableDepth = enableDepth;

    // Checking supported vulkan version
    {
        uint32_t version;
        VK_CHECK(vkEnumerateInstanceVersion(&version));
        bool supported = (VK_API_VERSION_MAJOR(version) == 1) && (VK_API_VERSION_MINOR(version) >= 2);
        if(!supported)
            MF_FATAL_ABORT(mfGetLogger(), "(From the vulkan backend) Minimum version support of Vulkan 1.2 is required!\n");
    }

    // Vulkan instance
    {
        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pApplicationName = appName,
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "MeltedForge",
            .apiVersion = VK_API_VERSION_1_2
        };

        u32 extCount = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
#ifdef MF_DEBUG
        // Checking if VK_LAYER_KHRONOS_validation is supported
        bool validationSupported = false;
        {
            u32 count = 0;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&count, mfnull));
            VkLayerProperties* props = MF_ALLOCMEM(VkLayerProperties, sizeof(VkLayerProperties) * count);
            VK_CHECK(vkEnumerateInstanceLayerProperties(&count, props));

            for(u32 i = 0; i < count; i++) {
                if(strcmp(props[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                    validationSupported = true;
                    break;
                }
            }

            MF_FREEMEM(props);
        }
        MF_PANIC_IF(!validationSupported, mfGetLogger(), "(From the vulkan backend) Validation layers not supported, but is required for debug builds!");

        const char** exts2 = MF_ALLOCMEM(const char*, sizeof(const char*) * (extCount + 1));
        memcpy(exts2, exts, sizeof(const char*) * extCount);
        exts2[extCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extCount++;
#endif

        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = extCount,
            .ppEnabledExtensionNames = exts
        };
#ifdef MF_DEBUG
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
            .pUserData = mfGetLogger(),
            .pfnUserCallback = debugCallback
        };

        info.pNext = &debugInfo;
        info.ppEnabledExtensionNames = exts2;
#endif

        VK_CHECK(vkCreateInstance(&info, ctx->allocator, &ctx->instance));
#ifdef MF_DEBUG
        MF_FREEMEM(exts2);
#endif
    }
    // Debug messenger
    {
#ifdef MF_DEBUG
        // Getting the functions
        {
            ctx->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
            ctx->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
            MF_PANIC_IF(!ctx->vkCreateDebugUtilsMessengerEXT || !ctx->vkDestroyDebugUtilsMessengerEXT, mfGetLogger(), 
                    "(From the vulkan backend) Failed to load the validation layer functions which are needed for debug builds!");
        }

        VkDebugUtilsMessengerCreateInfoEXT info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
            .pUserData = mfGetLogger(),
            .pfnUserCallback = debugCallback
        };

        VK_CHECK(ctx->vkCreateDebugUtilsMessengerEXT(ctx->instance, &info, ctx->allocator, &ctx->debugMessenger));
#endif
    }
    // Surface
    {
        VK_CHECK(glfwCreateWindowSurface(ctx->instance, mfWindowGetHandle(window), ctx->allocator, &ctx->surface));
    }
    // Physical Device 
    {
        u32 deviceCount = 0;
        VkResult result = vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, mfnull);
        if(result != VK_SUCCESS && result != VK_INCOMPLETE) {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_FATAL, "(From vulkan renderer backend) VkResult is %s (line: %d, function: %s, fileName: %s)", string_VkResult(result), __LINE__, __func__, __FILE__);
            abort();
        }
        VkPhysicalDevice* devices = MF_ALLOCMEM(VkPhysicalDevice, sizeof(VkPhysicalDevice) * deviceCount);
        result = vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);
        if(result != VK_SUCCESS && result != VK_INCOMPLETE) {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_FATAL, "(From vulkan renderer backend) VkResult is %s (line: %d, function: %s, fileName: %s)", string_VkResult(result), __LINE__, __func__, __FILE__);
            abort();
        }

        // TODO: Select the most capable physical device if there are more than one
        for(u32 i = 0; i < deviceCount; i++) {
            if(IsDeviceUsable(ctx->surface, devices[i])) {
                ctx->physicalDevice = devices[i];
                break;
            }
        }

        MF_FREEMEM(devices);
        MF_PANIC_IF(ctx->physicalDevice == mfnull, mfGetLogger(), "(From the vulkan backend) Failed to select a suitable GPU in the current PC!");

        ctx->queueData = GetDeviceQueueData(ctx->surface, ctx->physicalDevice);
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
                ctx->queueData.computeQueueIdx,
                ctx->queueData.presentQueueIdx,
                ctx->queueData.graphicsQueueIdx,
                ctx->queueData.transferQueueIdx
            };

            for(u32 i = 0; i < 4; i++) {
                bool isUnique = true;
    
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

        VkDeviceQueueCreateInfo* qInfos = MF_ALLOCMEM(VkDeviceQueueCreateInfo, sizeof(VkDeviceQueueCreateInfo) * queueCount);
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

        VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
            .scalarBlockLayout = VK_TRUE
        };

        VkDeviceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .enabledExtensionCount = deviceCount,
            .ppEnabledExtensionNames = deviceExts,
            .pEnabledFeatures = &features,
            .queueCreateInfoCount = queueCount,
            .pQueueCreateInfos = qInfos,
            .pNext = &scalarFeatures
        };

        VK_CHECK(vkCreateDevice(ctx->physicalDevice, &info, ctx->allocator, &ctx->device));
        MF_FREEMEM(qInfos);
    }
    // Queues 
    {
        vkGetDeviceQueue(ctx->device, (u32)ctx->queueData.computeQueueIdx, 0, &ctx->queueData.computeQueue);
        vkGetDeviceQueue(ctx->device, (u32)ctx->queueData.transferQueueIdx, 0, &ctx->queueData.transferQueue);
        vkGetDeviceQueue(ctx->device, (u32)ctx->queueData.presentQueueIdx, 0, &ctx->queueData.presentQueue);
        vkGetDeviceQueue(ctx->device, (u32)ctx->queueData.graphicsQueueIdx, 0, &ctx->queueData.graphicsQueue);
    }
    // Swapchain
    CreateSwapchain(ctx, mfWindowGetHandle(window));
    // Depth
    if(enableDepth) {
        GetDepthFormat(ctx);

        VulkanImageInfo info = {
            .ctx = ctx,
            .width = ctx->swapchainExtent.width,
            .height = ctx->swapchainExtent.height,
            .gpuResource = false,
            .pixels = mfnull,
            .format = ctx->depthFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .arrayLayers = 1,
            .type = VK_IMAGE_TYPE_2D
        };

        VulkanImageCreate(&ctx->depthImage, info);
    }
    // Global descriptor pool for shader resources
    {
        VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } 
		};

		VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1000 * MF_ARRAYLEN(poolSizes, VkDescriptorPoolSize),
            .poolSizeCount = MF_ARRAYLEN(poolSizes, VkDescriptorPoolSize),
            .pPoolSizes = poolSizes,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        };

        VK_CHECK(vkCreateDescriptorPool(ctx->device, &poolInfo, ctx->allocator, &ctx->uiDescriptorPool));
    }
    // Command Pool
    {
        ctx->commandPool = VulkanCommandPoolCreate(ctx, ctx->queueData.graphicsQueueIdx);
    }
    // Vam Allocator
    {
        VmaAllocatorCreateInfo info = {
            .instance = ctx->instance,
            .pAllocationCallbacks = ctx->allocator,
            .physicalDevice = ctx->physicalDevice,
            .device = ctx->device,
            .vulkanApiVersion = VK_API_VERSION_1_2,
            .pDeviceMemoryCallbacks = 0,
            .flags = 0
        };
        vmaCreateAllocator(&info, &ctx->vmaAllocator);
    }
}

void VulkanBackendCtxDestroy(VulkanBackendCtx* ctx) {
    VulkanCommandPoolDestroy(ctx, ctx->commandPool);
    vkDestroyDescriptorPool(ctx->device, ctx->uiDescriptorPool, ctx->allocator);

    if(ctx->enableDepth)
        VulkanImageDestroy(&ctx->depthImage);

    for(u32 i = 0; i < ctx->swapchainImageCount; i++)
        vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], ctx->allocator);
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, ctx->allocator);

    vmaDestroyAllocator(ctx->vmaAllocator);
    vkDestroyDevice(ctx->device, ctx->allocator);

#ifdef MF_DEBUG
    ctx->vkDestroyDebugUtilsMessengerEXT(ctx->instance, ctx->debugMessenger, ctx->allocator);
#endif

    vkDestroySurfaceKHR(ctx->instance, ctx->surface, ctx->allocator);
    vkDestroyInstance(ctx->instance, ctx->allocator);

    MF_FREEMEM(ctx->swapchainImageViews);
    MF_FREEMEM(ctx->swapchainImages);
    MF_SETMEM(ctx, 0, sizeof(VulkanBackendCtx));
}

void VulkanBackendCtxResize(VulkanBackendCtx* ctx, MFWindow* window) {
    VK_CHECK(vkDeviceWaitIdle(ctx->device));
    
    if(ctx->enableDepth)
        VulkanImageDestroy(&ctx->depthImage);

    for(u32 i = 0; i < ctx->swapchainImageCount; i++)
        vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], ctx->allocator);
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, ctx->allocator);

    CreateSwapchain(ctx, mfWindowGetHandle(window));

    if(ctx->enableDepth) {
        VulkanImageInfo info = {
            .ctx = ctx,
            .width = ctx->swapchainExtent.width,
            .height = ctx->swapchainExtent.height,
            .gpuResource = false,
            .pixels = mfnull,
            .format = ctx->depthFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .type = VK_IMAGE_TYPE_2D,
            .arrayLayers = 1,
            .viewType = VK_IMAGE_VIEW_TYPE_2D
        };

        VulkanImageCreate(&ctx->depthImage, info);
    }
}

#ifdef __cplusplus
}
#endif