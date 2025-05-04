#include "pipeline.h"

#include "common.h"

void VulkanPipelineCreate(VulkanBackendCtx* ctx, VulkanPipeline* pipeline, VulkanPipelineInfo info) {
    pipeline->info = info;

    VkPipelineLayoutCreateInfo layInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };

    VK_CHECK(vkCreatePipelineLayout(ctx->device, &layInfo, ctx->allocator, &pipeline->layout));

    VkGraphicsPipelineCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
    };

    VK_CHECK(vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &info, ctx->allocator, &pipeline->pipeline));    
}

void VulkanPipelineDestroy(VulkanBackendCtx* ctx, VulkanPipeline* pipeline) {
    vkDestroyPipeline(ctx->device, pipeline->pipeline, ctx->allocator);
    vkDestroyPipelineLayout(ctx->device, pipeline->layout, ctx->allocator);
}