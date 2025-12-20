#pragma once

#include <vulkan/vulkan.h>

#include "ctx.h"

typedef struct VulkanPipelineInfo_s {
    VkRenderPass pass;
    VkExtent2D extent;
    u32 bindingDescsCount, attribDescsCount, setLayoutCount;
    VkVertexInputBindingDescription* bindingDescs;
    VkVertexInputAttributeDescription* attribDescs;
    VkDescriptorSetLayout* setLayouts;
    VkCompareOp depthCompareOp;
    b8 hasDepth, transparent;
    const char* vertPath;
    const char* fragPath;
} VulkanPipelineInfo;

typedef struct VulkanPipeline_s {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkPipelineBindPoint bindPoint;
    VulkanPipelineInfo info;
} VulkanPipeline;

void VulkanPipelineCreate(VulkanBackendCtx* ctx, VulkanPipeline* pipeline, VulkanPipelineInfo* info);
void VulkanPipelineDestroy(VulkanBackendCtx* ctx, VulkanPipeline* pipeline);

void VulkanPipelineBind(VulkanPipeline* pipeline, VkViewport vp, VkRect2D scissor, VkCommandBuffer buffer);