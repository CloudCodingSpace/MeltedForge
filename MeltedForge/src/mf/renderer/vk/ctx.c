#ifdef __cplusplus
extern "C" {
#endif

#include "ctx.h"

#include <GLFW/glfw3.h>

#include <string.h>
#include <vulkan/vulkan_core.h>

#include "core/mfarray.h"

#include "common.h"
#include "command_buffer.h"

static VkBool32 debugCallback(
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
    return VK_FALSE;
}

static VulkanBackendQueueData GetDeviceQueueData(VulkanBackendCtx* ctx, VkSurfaceKHR surface, VkPhysicalDevice device) {
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

        if(ctx->config.headless) {
            data.presentQueueIdx = 0; // HACK: To bypass the IsQueueDataComplete
            continue;
        }
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

static bool IsDeviceUsable(VulkanBackendCtx* ctx, VkSurfaceKHR surface, VkPhysicalDevice device) {
    VulkanBackendQueueData data = GetDeviceQueueData(ctx, surface, device);

    bool extSupport = false;
    if(!ctx->config.headless) {
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
    } else {
        return IsQueueDataComplete(data);
    }

    return IsQueueDataComplete(data) && extSupport;
}

static MFOptionalRenderFeatures GetPhysicalDeviceRenderFeatures(VkPhysicalDevice device) {
    MFOptionalRenderFeatures featureFlags = MF_OPTIONAL_RENDER_FEATURE_MAX_ENUM;

    VkPhysicalDeviceVulkan12Features vk12Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    };

    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vk12Features
    };
    vkGetPhysicalDeviceFeatures2(device, &features);

    if(features.features.samplerAnisotropy)
        featureFlags |= MF_OPTIONAL_RENDER_FEATURE_SAMPLER_ANISOTROPY;
    if(vk12Features.descriptorIndexing)
        featureFlags |= MF_OPTIONAL_RENDER_FEATURE_DESCRIPTOR_INDEXING;
    if(vk12Features.scalarBlockLayout)
        featureFlags |= MF_OPTIONAL_RENDER_FEATURE_SCALAR_LAYOUT;
    if(vk12Features.runtimeDescriptorArray)
        featureFlags |= MF_OPTIONAL_RENDER_FEATURE_VARIABLE_DESCRIPTOR_SIZES;
    if(vk12Features.shaderSampledImageArrayNonUniformIndexing && vk12Features.shaderStorageImageArrayNonUniformIndexing && vk12Features.shaderStorageBufferArrayNonUniformIndexing && vk12Features.shaderUniformBufferArrayNonUniformIndexing)
        featureFlags |= MF_OPTIONAL_RENDER_FEATURE_SHADER_NON_UNIFORM_ACCESS;
    if(vk12Features.bufferDeviceAddress)
        featureFlags |= MF_OPTIONAL_RENDER_FEATURE_BUFFER_DEVICE_ADDRESS;

    return featureFlags;
}

static u64 RatePhysicalDevice(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &features);
    vkGetPhysicalDeviceMemoryProperties(device, &memory);

    MFOptionalRenderFeatures featureFlags = GetPhysicalDeviceRenderFeatures(device);

    u64 score = 0;
    switch(props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 5000;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 2000;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 1000;
            break;

        default:
            score += 100;
    }

    if(features.multiDrawIndirect)
        score += 200;
    if(features.occlusionQueryPrecise)
        score += 200;
    if(features.shaderSampledImageArrayDynamicIndexing)
        score += 200;
    if(features.shaderStorageBufferArrayDynamicIndexing)
        score += 200;
    if(features.shaderStorageImageArrayDynamicIndexing)
        score += 200;
    if(features.shaderStorageImageExtendedFormats)
        score += 200;

    if((featureFlags & MF_OPTIONAL_RENDER_FEATURE_SAMPLER_ANISOTROPY) == MF_OPTIONAL_RENDER_FEATURE_SAMPLER_ANISOTROPY)
        score += 800;
    if((featureFlags & MF_OPTIONAL_RENDER_FEATURE_SCALAR_LAYOUT) == MF_OPTIONAL_RENDER_FEATURE_SCALAR_LAYOUT)
        score += 800;
    if((featureFlags & MF_OPTIONAL_RENDER_FEATURE_BUFFER_DEVICE_ADDRESS) == MF_OPTIONAL_RENDER_FEATURE_BUFFER_DEVICE_ADDRESS)
        score += 800;
    if((featureFlags & MF_OPTIONAL_RENDER_FEATURE_DESCRIPTOR_INDEXING) == MF_OPTIONAL_RENDER_FEATURE_DESCRIPTOR_INDEXING)
        score += 800;
    if((featureFlags & MF_OPTIONAL_RENDER_FEATURE_VARIABLE_DESCRIPTOR_SIZES) == MF_OPTIONAL_RENDER_FEATURE_VARIABLE_DESCRIPTOR_SIZES)
        score += 800;
    if((featureFlags & MF_OPTIONAL_RENDER_FEATURE_SHADER_NON_UNIFORM_ACCESS) == MF_OPTIONAL_RENDER_FEATURE_SHADER_NON_UNIFORM_ACCESS)
        score += 800;

    if(features.geometryShader)
        score += 600;
    if(features.tessellationShader)
        score += 600;

    score += VK_API_VERSION_MAJOR(props.apiVersion) * 1000;
    score += VK_API_VERSION_MINOR(props.apiVersion) * 100;

    return score;
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

static void SelectScCaps(VulkanBackendCtx* ctx, VulkanScCaps caps) {
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
            if(ctx->config.vsync) {
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
			if (caps.formats[i].format == VK_FORMAT_R8G8B8A8_UNORM && caps.formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
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
			glfwGetFramebufferSize(ctx->config.window, &width, &height);

			ctx->swapchainExtent = (VkExtent2D) {
				(u32)width,
				(u32)height
			};

			ctx->swapchainExtent.width = MF_CLAMP(ctx->swapchainExtent.width, caps.caps.minImageExtent.width, caps.caps.maxImageExtent.width);
			ctx->swapchainExtent.height = MF_CLAMP(ctx->swapchainExtent.height, caps.caps.minImageExtent.height, caps.caps.maxImageExtent.height);
		}
	}
}

static void CreateSwapchain(VulkanBackendCtx* ctx) {
    if(ctx->config.headless) {
        ctx->swapchainImageCount = FRAMES_IN_FLIGHT;
        ctx->swapchainImages = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * ctx->swapchainImageCount);
        ctx->swapchainFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
        ctx->swapchainExtent.width = ctx->config.headlessExtent.x;
        ctx->swapchainExtent.height = ctx->config.headlessExtent.y;
    
        for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
            VulkanImageInfo info = {
                .gpuResource = false,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .arrayLayers = 1,
                .mipLevels = 1,
                .generateMipmaps = false,
                .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .width = ctx->swapchainExtent.width,
                .height = ctx->swapchainExtent.height,
                .format = ctx->swapchainFormat.format,
                .ctx = ctx,
                .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                .type = VK_IMAGE_TYPE_2D,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .samples = VK_SAMPLE_COUNT_1_BIT
            };

            VulkanImageCreate(&ctx->swapchainImages[i], info);
        }
    } else {
        VulkanScCaps caps = GetScCaps(ctx);

        SelectScCaps(ctx, caps);
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
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .clipped = VK_TRUE,
                .oldSwapchain = mfnull
            };

            if (ctx->uniqueQueueCount > 1) {
                info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                info.queueFamilyIndexCount = ctx->uniqueQueueCount;
                info.pQueueFamilyIndices = ctx->uniqueQueues;
            } else {
                info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            VK_CHECK(vkCreateSwapchainKHR(ctx->device, &info, ctx->allocator, &ctx->swapchain));
        }
        // Getting the swapchain images and creating image views
        {
            VK_CHECK(vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, mfnull));
            VkImage* swapchainImages = MF_ALLOCMEM(VkImage, sizeof(VkImage) * ctx->swapchainImageCount);
            ctx->swapchainImages = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * ctx->swapchainImageCount);
            VK_CHECK(vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, swapchainImages));

            for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
                ctx->swapchainImages[i] = (VulkanImage) {
                    .info = {
                        .ctx = ctx,
                        .arrayLayers = 1,
                        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                        .format = ctx->swapchainFormat.format,
                        .generateMipmaps = false,
                        .gpuResource = false,
                        .width = ctx->swapchainExtent.width,
                        .height = ctx->swapchainExtent.height,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .mipLevels = 1,
                        .tiling = VK_IMAGE_TILING_OPTIMAL,
                        .type = VK_IMAGE_TYPE_2D,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                        .memFlags = VMA_MEMORY_USAGE_GPU_ONLY
                    },
                    .access = 0,
                    .stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    .layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .image = swapchainImages[i],
                    .cmdBuff = VulkanCommandBufferAllocate(ctx, ctx->commandPool, true)
                };
                {
                    VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
                    VK_CHECK(vkCreateFence(ctx->device, &info, ctx->allocator, &ctx->swapchainImages[i].fence));
                }
                
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
                    .image = ctx->swapchainImages[i].image
                };

                VK_CHECK(vkCreateImageView(ctx->device, &info, ctx->allocator, &ctx->swapchainImages[i].view));
            }
            MF_FREEMEM(swapchainImages);
        }

        MF_FREEMEM(caps.formats);
        MF_FREEMEM(caps.modes);
    }
}

static VkSampleCountFlagBits GetMaxSupportedSampleCount(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(device, &props);

    VkSampleCountFlagBits samples = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
    if (samples & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (samples & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (samples & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (samples & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (samples & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (samples & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanBackendCtxInit(VulkanBackendCtx* ctx, VulkanBackendCtxConfig config) {
    ctx->allocator = mfnull; // TODO: Create a custom allocator
    ctx->config = config;

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
            .pApplicationName = ctx->config.appName,
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
    if(!ctx->config.headless) {
        VK_CHECK(glfwCreateWindowSurface(ctx->instance, config.window, ctx->allocator, &ctx->surface));
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

        MFArray usableDevices = mfArrayCreate(1, sizeof(VkPhysicalDevice));
        MFArray usableScores = mfArrayCreate(1, sizeof(u64));
        for(u32 i = 0; i < deviceCount; i++) {
            if(IsDeviceUsable(ctx, ctx->surface, devices[i])) {
                mfArrayAddElement(&usableDevices, VkPhysicalDevice, devices[i]);
                mfArrayAddElement(&usableScores, u64, RatePhysicalDevice(devices[i]));
            }
        }

        MF_PANIC_IF(usableDevices.len == 0, mfGetLogger(), "(From the vulkan backend) Failed to select a suitable GPU in the current PC!");
        
        u64 highestRate = 0;
        for(u64 i = 0; i < usableDevices.len; i++) {
            if(highestRate < mfArrayGetElement(usableScores, u64, i)) {
                highestRate = mfArrayGetElement(usableScores, u64, i);
                ctx->physicalDevice = mfArrayGetElement(usableDevices, VkPhysicalDevice, i);
            }
        }

        MF_PANIC_IF(ctx->physicalDevice == mfnull, mfGetLogger(), "(From the vulkan backend) Failed to select a suitable GPU in the current PC!");

        mfArrayDestroy(&usableDevices);
        mfArrayDestroy(&usableScores);
        MF_FREEMEM(devices);

        ctx->queueData = GetDeviceQueueData(ctx, ctx->surface, ctx->physicalDevice);
        ctx->maxSupportedSamples = GetMaxSupportedSampleCount(ctx->physicalDevice);
        ctx->samples = (ctx->maxSupportedSamples >= config.samples) ? config.samples : ctx->maxSupportedSamples;
        ctx->featureFlags = GetPhysicalDeviceRenderFeatures(ctx->physicalDevice);
        
        {
            ctx->features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            VkPhysicalDeviceFeatures2 features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &ctx->features
            };
            vkGetPhysicalDeviceFeatures2(ctx->physicalDevice, &features);
        
            ctx->props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
            VkPhysicalDeviceProperties2 props = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &ctx->props
            };
            vkGetPhysicalDeviceProperties2(ctx->physicalDevice, &props);
        }
    }
    // Device 
    {
        u32 extCount = 1;
        const char* deviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        ctx->uniqueQueueCount = 0;
        MF_SETMEM(ctx->uniqueQueues, 0, sizeof(u32) * 4);
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
    
                for(u32 j = 0; j < ctx->uniqueQueueCount; j++) {
                    if(qs[i] == ctx->uniqueQueues[j]) {
                        isUnique = false;
                        break;
                    }
                }
    
                if(isUnique) {
                    ctx->uniqueQueues[ctx->uniqueQueueCount++] = qs[i];
                }
            }
        }

        float qPriority = 1.0f;
        VkDeviceQueueCreateInfo* qInfos = MF_ALLOCMEM(VkDeviceQueueCreateInfo, sizeof(VkDeviceQueueCreateInfo) * ctx->uniqueQueueCount);
        MF_SETMEM(qInfos, 0, sizeof(VkDeviceQueueCreateInfo) * ctx->uniqueQueueCount);
        for(u32 i = 0; i < ctx->uniqueQueueCount; i++) {
            qInfos[i] = (VkDeviceQueueCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pQueuePriorities = &qPriority,
                .queueCount = 1,
                .queueFamilyIndex = ctx->uniqueQueues[i]
            };
        }

        VkPhysicalDeviceVulkan12Features vk12Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        };

        VkPhysicalDeviceFeatures2 features2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &vk12Features
        };
        vkGetPhysicalDeviceFeatures2(ctx->physicalDevice, &features2);

        VkDeviceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .enabledExtensionCount = extCount,
            .ppEnabledExtensionNames = deviceExts,
            .pEnabledFeatures = &features2.features,
            .queueCreateInfoCount = ctx->uniqueQueueCount,
            .pQueueCreateInfos = qInfos,
            .pNext = &vk12Features
        };

        if(ctx->config.headless)
            info.enabledExtensionCount = 0;

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
    // Command Pool
    {
        ctx->commandPool = VulkanCommandPoolCreate(ctx, ctx->queueData.graphicsQueueIdx);
        ctx->computeCommandPool = VulkanCommandPoolCreate(ctx, ctx->queueData.computeQueueIdx);
    }
    // Vma Allocator
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
        VK_CHECK(vmaCreateAllocator(&info, &ctx->vmaAllocator));
    }
    // Swapchain
    CreateSwapchain(ctx);
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
            .maxSets = 1000 * MF_ARRAYLEN(poolSizes),
            .poolSizeCount = MF_ARRAYLEN(poolSizes),
            .pPoolSizes = poolSizes,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        };

        VK_CHECK(vkCreateDescriptorPool(ctx->device, &poolInfo, ctx->allocator, &ctx->uiDescriptorPool));
    }
}

void VulkanBackendCtxDestroy(VulkanBackendCtx* ctx) {
    for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
        if(!ctx->config.headless)
            vkDestroyImageView(ctx->device, ctx->swapchainImages[i].view, ctx->allocator);
        else
            VulkanImageDestroy(&ctx->swapchainImages[i]);
        vkDestroyFence(ctx->device, ctx->swapchainImages[i].fence, ctx->allocator);
    }
    
    VulkanCommandPoolDestroy(ctx, ctx->commandPool);
    VulkanCommandPoolDestroy(ctx, ctx->computeCommandPool);
    vkDestroyDescriptorPool(ctx->device, ctx->uiDescriptorPool, ctx->allocator);

    if(!ctx->config.headless)
        vkDestroySwapchainKHR(ctx->device, ctx->swapchain, ctx->allocator);
    else
        MF_FREEMEM(ctx->swapchainImages);

    vmaDestroyAllocator(ctx->vmaAllocator);
    vkDestroyDevice(ctx->device, ctx->allocator);

#ifdef MF_DEBUG
    ctx->vkDestroyDebugUtilsMessengerEXT(ctx->instance, ctx->debugMessenger, ctx->allocator);
#endif

    if(!ctx->config.headless)
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, ctx->allocator);
    vkDestroyInstance(ctx->instance, ctx->allocator);

    MF_FREEMEM(ctx->swapchainImages);
    MF_SETMEM(ctx, 0, sizeof(VulkanBackendCtx));
}

void VulkanBackendCtxResize(VulkanBackendCtx* ctx) {
    VK_CHECK(vkDeviceWaitIdle(ctx->device));
   
    for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
        if(!ctx->config.headless)
            vkDestroyImageView(ctx->device, ctx->swapchainImages[i].view, ctx->allocator);
        else
            VulkanImageDestroy(&ctx->swapchainImages[i]);
        VulkanCommandBufferFree(ctx, ctx->swapchainImages[i].cmdBuff, ctx->commandPool);
        vkDestroyFence(ctx->device, ctx->swapchainImages[i].fence, ctx->allocator);
    }
    MF_FREEMEM(ctx->swapchainImages);
    
    if(!ctx->config.headless) {
        vkDestroySwapchainKHR(ctx->device, ctx->swapchain, ctx->allocator);
    } else {
        MF_FREEMEM(ctx->swapchainImages);
    }
    CreateSwapchain(ctx);
}

#ifdef __cplusplus
}
#endif
