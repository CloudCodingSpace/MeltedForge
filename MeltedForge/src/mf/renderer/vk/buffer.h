#pragma once

#include "common.h"
#include "ctx.h"

#include "core/mfutils.h"

typedef enum VulkanBufferTypes_e {
    VULKAN_BUFFER_TYPE_NONE,
    VULKAN_BUFFER_TYPE_VERTEX,
    VULKAN_BUFFER_TYPE_INDEX,
    VULKAN_BUFFER_TYPE_STAGING
} VulkanBufferTypes;

typedef struct VulkanBuffer_s {
    VkBuffer handle;
    VkDeviceMemory mem;
    u64 size;
    void* data;
    VulkanBufferTypes type;
} VulkanBuffer;

void VulkanBufferAllocate(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, u64 size, void* data, VulkanBufferTypes type);
void VulkanBufferFree(VulkanBuffer* buffer, VulkanBackendCtx* ctx);

void VulkanBufferResize(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, u64 size, void* data);
void VulkanBufferUploadData(VulkanBuffer* buffer, VulkanBackendCtx* ctx, VkCommandPool pool, void* data);