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
}

MF_INLINE u32 VulkanFormatBytesPerPixel(VkFormat format) {
    switch(format) {
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
    }

    const char* name = string_VkFormat(format);

    u32 bytes = 0;

    if(strstr(name, "R8")) bytes += 1;
    if(strstr(name, "G8")) bytes += 1;
    if(strstr(name, "B8")) bytes += 1;
    if(strstr(name, "A8")) bytes += 1;

    if(strstr(name, "R16")) bytes += 2;
    if(strstr(name, "G16")) bytes += 2;
    if(strstr(name, "B16")) bytes += 2;
    if(strstr(name, "A16")) bytes += 2;

    if(strstr(name, "R32")) bytes += 4;
    if(strstr(name, "G32")) bytes += 4;
    if(strstr(name, "B32")) bytes += 4;
    if(strstr(name, "A32")) bytes += 4;

    return bytes;
}

#define VK_CHECK(result) check_vk_result(result, __LINE__, __func__, __FILE__)
#define FRAMES_IN_FLIGHT 2

#ifdef __cplusplus
}
#endif