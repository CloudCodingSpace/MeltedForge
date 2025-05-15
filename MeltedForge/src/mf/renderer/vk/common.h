#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "../mfutil_types.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdio.h>
#include <stdlib.h>

MF_INLINE void check_vk_result(VkResult result, uint32_t lineNum, const char* funcName, const char* fileName) {
    if (result != VK_SUCCESS) {
        slogLoggerSetColor(mfGetLogger(), SLCOLOR_RED);
        printf("(From vulkan renderer backend) VkResult is %s (line: %d, function: %s, fileName: %s)", string_VkResult(result), lineNum, funcName, fileName);
        slogLoggerSetColor(mfGetLogger(), SLCOLOR_DEFAULT);
        abort();
    }
}

MF_INLINE uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    return 0xffffffff;
}

#define VK_CHECK(result) check_vk_result(result, __LINE__, __func__, __FILE__)
#define FRAMES_IN_FLIGHT 2