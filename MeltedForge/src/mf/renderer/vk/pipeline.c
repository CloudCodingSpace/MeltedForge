#include "pipeline.h"

#include "common.h"

void VulkanPipelineCreate(VulkanBackendCtx* ctx, VulkanPipeline* pipeline, VulkanPipelineInfo info) {
    pipeline->info = info;

    VkPipelineLayoutCreateInfo layInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };

    VK_CHECK(vkCreatePipelineLayout(ctx->device, &layInfo, ctx->allocator, &pipeline->layout));

    VkPipelineColorBlendAttachmentState blendState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };

    VkPipelineColorBlendStateCreateInfo blendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendState
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkPipelineVertexInputStateCreateInfo vertState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexAttributeDescriptionCount = info.attribDescsCount,
        .pVertexAttributeDescriptions = info.attribDescs,
        .vertexBindingDescriptionCount = info.bindingDescsCount,
        .pVertexBindingDescriptions = info.bindingDescs
    };

    VkPipelineInputAssemblyStateCreateInfo inputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .primitiveRestartEnable = VK_FALSE, // TODO: Make it configurable
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST // TODO: Make it configurable
    };

    VkPipelineMultisampleStateCreateInfo msaaState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // TODO: Make it configurable
        .sampleShadingEnable = VK_FALSE // TODO: Make it configurable
    };

    VkPipelineRasterizationStateCreateInfo rasState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_BACK_BIT, // TODO: Make it configurable
        .frontFace = VK_FRONT_FACE_CLOCKWISE, // TODO: Make it configurable
        .depthBiasEnable = VK_FALSE, // TODO: Make it configurable
        .depthClampEnable = VK_FALSE, // TODO: Make it configurable
        .lineWidth = 1.0f, // TODO: Make it configurable
        .polygonMode = VK_POLYGON_MODE_FILL, // TODO: Make it configurable
        .rasterizerDiscardEnable = VK_FALSE // TODO: Make it configurable
    };

    VkRect2D scissor = {
        .extent = info.extent,
        .offset = (VkOffset2D){.x = 0, .y = 0}
    };

    VkViewport vp = {
        .x = 0,
        .y = 0,
        .maxDepth = 1.0f,
        .minDepth = 0.0f,
        .width = info.extent.width,
        .height = info.extent.height
    };

    VkPipelineViewportStateCreateInfo vpState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .scissorCount = 1,
        .pScissors = &scissor,
        .viewportCount = 1,
        .pViewports = &vp
    };

    VkPipelineDepthStencilStateCreateInfo depthState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS // TODO: Make it configurable
    };

    VkGraphicsPipelineCreateInfo ginfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .basePipelineHandle = mfnull,
        .basePipelineIndex = -1,
        .layout = pipeline->layout,
        .renderPass = info.pass,
        .subpass = 0,
        .pColorBlendState = &blendInfo,
        .pDynamicState = &dInfo,
        .pVertexInputState = &vertState,
        .pInputAssemblyState = &inputState,
        .pMultisampleState = &msaaState,
        .pRasterizationState = &rasState,
        .pViewportState = &vpState
    };

    if(info.hasDepth)
        ginfo.pDepthStencilState = &depthState;

    VK_CHECK(vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &ginfo, ctx->allocator, &pipeline->pipeline));    
}

void VulkanPipelineDestroy(VulkanBackendCtx* ctx, VulkanPipeline* pipeline) {
    vkDestroyPipeline(ctx->device, pipeline->pipeline, ctx->allocator);
    vkDestroyPipelineLayout(ctx->device, pipeline->layout, ctx->allocator);
}