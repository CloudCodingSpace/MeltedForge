#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "../mfutil_types.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MF_INLINE void check_vk_result(VkResult result, uint32_t lineNum, const char* funcName, const char* fileName) {
    if (result != VK_SUCCESS) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_FATAL, "(From vulkan renderer backend) VkResult is %s (line: %d, function: %s, fileName: %s)", string_VkResult(result), lineNum, funcName, fileName);
        abort();
    }
}

MF_INLINE u32 FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    MF_FATAL_ABORT(mfGetLogger(), "(From the vulkan backend) Failed to find the suitable memory type!");
}

MF_INLINE u32 VulkanFormatBytesPerPixel(VkFormat format) {
    switch(format) {
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_USCALED: return 3;
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_USCALED: return 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return 8;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
        default:
            MF_FATAL_ABORT(mfGetLogger(), "(From the vulkan backend) The format provided isn't supported or is invalid! VkFormat is %s", string_VkFormat(format));
    }
}

#define VK_CHECK(result) check_vk_result(result, __LINE__, __func__, __FILE__)
#define FRAMES_IN_FLIGHT 2

#ifdef __cplusplus
}
#endif