#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#include "ctx.h"

typedef struct VulkanPipelineInfo_s {
    VkRenderPass renderpass;
    VkExtent2D extent;
    u32 bindingsCount, attributesCount, setLayoutCount, pushConstRangesCount;
    VkVertexInputBindingDescription* bindings;
    VkVertexInputAttributeDescription* attributes;
    VkPushConstantRange* pushConstRanges;
    VkDescriptorSetLayout* setLayouts;
    VkCompareOp depthCompareOp;
    VkPipelineCache cache;
    bool hasDepth, transparent;
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

#ifdef __cplusplus
}
#endif