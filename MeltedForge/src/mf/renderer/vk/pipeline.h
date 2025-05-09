#pragma once

#include <vulkan/vulkan.h>

#include "ctx.h"

typedef struct VulkanPipelineInfo_s {
    VkRenderPass pass;
    VkExtent2D extent;
    u32 bindingDescsCount, attribDescsCount;
    VkVertexInputBindingDescription* bindingDescs;
    VkVertexInputAttributeDescription* attribDescs;
    b8 hasDepth;
} VulkanPipelineInfo;

typedef struct VulkanPipeline_s {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkPipelineBindPoint bindPoint;
    VulkanPipelineInfo info;
} VulkanPipeline;

void VulkanPipelineCreate(VulkanBackendCtx* ctx, VulkanPipeline* pipeline, VulkanPipelineInfo info);
void VulkanPipelineDestroy(VulkanBackendCtx* ctx, VulkanPipeline* pipeline);