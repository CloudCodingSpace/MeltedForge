#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#include "ctx.h"

typedef enum VulkanPipelineType_e {
    VULKAN_PIPELINE_TYPE_GRAPHICS,
    VULKAN_PIPELINE_TYPE_COMPUTE
} VulkanPipelineType;

typedef struct VulkanGPipelineInfo_s {
    VkRenderPass renderpass;
    VkExtent2D extent;
    u32 bindingsCount, attributesCount;
    VkVertexInputBindingDescription* bindings;
    VkVertexInputAttributeDescription* attributes;
    VkCompareOp depthCompareOp;
    VkCullModeFlags cullMode;
    VkSampleCountFlagBits samples;
    bool hasDepth, transparent;
    const char* vertPath;
    const char* fragPath;
} VulkanGPipelineInfo;

typedef struct VulkanCPipelineInfo_s {
    const char* path;
} VulkanCPipelineInfo;

typedef struct VulkanPipelineInfo_s {
    u32 setLayoutCount, pushConstRangesCount;
    VkPushConstantRange* pushConstRanges;
    VkDescriptorSetLayout* setLayouts;
    VkPipelineCache cache;

    VulkanPipelineType type;
    VulkanGPipelineInfo ginfo;
    VulkanCPipelineInfo cinfo;
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