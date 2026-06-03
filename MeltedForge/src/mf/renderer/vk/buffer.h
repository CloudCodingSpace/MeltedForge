#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "ctx.h"

#include "core/mfutils.h"

typedef enum VulkanBufferTypes_e {
    VULKAN_BUFFER_TYPE_NONE,
    VULKAN_BUFFER_TYPE_VERTEX,
    VULKAN_BUFFER_TYPE_INDEX,
    VULKAN_BUFFER_TYPE_UBO,
    VULKAN_BUFFER_TYPE_SSBO,
    VULKAN_BUFFER_TYPE_STAGING
} VulkanBufferTypes;

typedef struct VulkanBufferInfo_s {
    VulkanBackendCtx* ctx;
    VkCommandPool pool;
    u64 size;
    void* data;
    VulkanBufferTypes type;
    bool frequentUpdates;
} VulkanBufferInfo;

typedef struct VulkanBuffer_s {
    VkBuffer handle;
    VmaAllocation allocation;
    void* mappedMem;
    VulkanBufferInfo info;
} VulkanBuffer;

void VulkanBufferAllocate(VulkanBuffer* buffer, VulkanBufferInfo info);
void VulkanBufferFree(VulkanBuffer* buffer);

void VulkanBufferResize(VulkanBuffer* buffer, u64 newSize);
void VulkanBufferUploadData(VulkanBuffer* buffer, void* data);

#ifdef __cplusplus
}
#endif