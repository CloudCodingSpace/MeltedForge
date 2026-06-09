#ifdef __cplusplus
extern "C" {
#endif

#include "pipeline.h"

#include "common.h"

void VulkanPipelineCreate(VulkanBackendCtx* ctx, VulkanPipeline* pipeline, VulkanPipelineInfo* info) {
    pipeline->info = *info;

    VkPipelineLayoutCreateInfo layInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = info->setLayoutCount,
        .pSetLayouts = info->setLayouts,
        .pushConstantRangeCount = info->pushConstRangesCount,
        .pPushConstantRanges = info->pushConstRanges
    };

    VK_CHECK(vkCreatePipelineLayout(ctx->device, &layInfo, ctx->allocator, &pipeline->layout));

    if(pipeline->info.type == VULKAN_PIPELINE_TYPE_GRAPHICS) {
        pipeline->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    
        VkPipelineColorBlendAttachmentState blendState = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE
        };

        if(info->ginfo.transparent) {
            blendState.blendEnable = VK_TRUE,
            blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            blendState.colorBlendOp = VK_BLEND_OP_ADD,
            blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            blendState.alphaBlendOp = VK_BLEND_OP_ADD;
        }

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
            .vertexAttributeDescriptionCount = info->ginfo.attributesCount,
            .pVertexAttributeDescriptions = info->ginfo.attributes,
            .vertexBindingDescriptionCount = info->ginfo.bindingsCount,
            .pVertexBindingDescriptions = info->ginfo.bindings
        };

        VkPipelineInputAssemblyStateCreateInfo inputState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .primitiveRestartEnable = VK_FALSE, // NOTE: Make it configurable
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST // NOTE: Make it configurable
        };

        VkPipelineMultisampleStateCreateInfo msaaState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE // NOTE: Make it configurable
        };
        msaaState.rasterizationSamples = info->ginfo.samples;

        VkPipelineRasterizationStateCreateInfo rasState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .cullMode = info->ginfo.cullMode,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, // NOTE: Make it configurable
            .depthBiasEnable = VK_FALSE, // NOTE: Make it configurable
            .depthClampEnable = VK_FALSE, // NOTE: Make it configurable
            .lineWidth = 1.0f, // NOTE: Make it configurable
            .polygonMode = VK_POLYGON_MODE_FILL, // NOTE: Make it configurable
            .rasterizerDiscardEnable = VK_FALSE // NOTE: Make it configurable
        };

        VkRect2D scissor = {
            .extent = info->ginfo.extent,
            .offset = (VkOffset2D){.x = 0, .y = 0}
        };

        VkViewport vp = {
            .x = 0,
            .y = 0,
            .maxDepth = 1.0f,
            .minDepth = 0.0f,
            .width = info->ginfo.extent.width,
            .height = info->ginfo.extent.height
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
            .depthCompareOp = VK_COMPARE_OP_LESS
        };

        if((((int)info->ginfo.depthCompareOp) <= VK_COMPARE_OP_ALWAYS) && (((int)info->ginfo.depthCompareOp) >= 0)) {
            depthState.depthCompareOp = info->ginfo.depthCompareOp;
        }

        VkShaderModule vertMod, fragMod;
        // Modules
        {
            size_t vertSize, fragSize;
            bool success = false;
            char* vertCode = mfReadFile(mfGetLogger(), &vertSize, &success, info->ginfo.vertPath, "rb");
            MF_PANIC_IF(!success, mfGetLogger(), "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");
            char* fragCode = mfReadFile(mfGetLogger(), &fragSize, &success, info->ginfo.fragPath, "rb");
            MF_PANIC_IF(!success, mfGetLogger(), "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");

            VkShaderModuleCreateInfo vertInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = vertSize,
                .pCode = (u32*)vertCode
            };

            VkShaderModuleCreateInfo fragInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = fragSize,
                .pCode = (u32*)fragCode
            };

            VK_CHECK(vkCreateShaderModule(ctx->device, &vertInfo, ctx->allocator, &vertMod));
            VK_CHECK(vkCreateShaderModule(ctx->device, &fragInfo, ctx->allocator, &fragMod));

            MF_FREEMEM(vertCode);
            MF_FREEMEM(fragCode);
        }

        VkPipelineShaderStageCreateInfo stages[2];
        stages[0] = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertMod,
            .pName = "main"
        };

        stages[1] = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragMod,
            .pName = "main"
        };

        VkGraphicsPipelineCreateInfo ginfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .basePipelineHandle = mfnull,
            .basePipelineIndex = -1,
            .layout = pipeline->layout,
            .renderPass = info->ginfo.renderpass,
            .subpass = 0,
            .pColorBlendState = &blendInfo,
            .pDynamicState = &dInfo,
            .pVertexInputState = &vertState,
            .pInputAssemblyState = &inputState,
            .pMultisampleState = &msaaState,
            .pRasterizationState = &rasState,
            .pViewportState = &vpState,
            .stageCount = 2,
            .pStages = stages
        };

        if(info->ginfo.hasDepth)
            ginfo.pDepthStencilState = &depthState;

        VK_CHECK(vkCreateGraphicsPipelines(ctx->device, info->cache, 1, &ginfo, ctx->allocator, &pipeline->pipeline));    

        vkDestroyShaderModule(ctx->device, vertMod, ctx->allocator);
        vkDestroyShaderModule(ctx->device, fragMod, ctx->allocator);
    } else if(pipeline->info.type == VULKAN_PIPELINE_TYPE_COMPUTE) {
        pipeline->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    
        VkShaderModule mod = VK_NULL_HANDLE;
        // Modules
        {
            size_t size;
            bool success = false;
            char* code = mfReadFile(mfGetLogger(), &size, &success, info->cinfo.path, "rb");
            MF_PANIC_IF(!success, mfGetLogger(), "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");

            VkShaderModuleCreateInfo vertInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = size,
                .pCode = (u32*)code
            };

            VK_CHECK(vkCreateShaderModule(ctx->device, &vertInfo, ctx->allocator, &mod));

            MF_FREEMEM(code);
        }

        VkPipelineShaderStageCreateInfo stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = mod,
            .pName = "main",
            .stage = VK_SHADER_STAGE_COMPUTE_BIT
        };

        VkComputePipelineCreateInfo cinfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .basePipelineIndex = -1,
            .layout = pipeline->layout,
            .stage = stage
        };

        VK_CHECK(vkCreateComputePipelines(ctx->device, info->cache, 1, &cinfo, ctx->allocator, &pipeline->pipeline));

        vkDestroyShaderModule(ctx->device, mod, ctx->allocator);
    }
}

void VulkanPipelineDestroy(VulkanBackendCtx* ctx, VulkanPipeline* pipeline) {
    vkDestroyPipeline(ctx->device, pipeline->pipeline, ctx->allocator);
    vkDestroyPipelineLayout(ctx->device, pipeline->layout, ctx->allocator);
}

void VulkanPipelineBind(VulkanPipeline* pipeline, VkViewport vp, VkRect2D scissor, VkCommandBuffer buffer) {
    vkCmdBindPipeline(buffer, pipeline->bindPoint, pipeline->pipeline);

    vkCmdSetViewport(buffer, 0, 1, &vp);
    vkCmdSetScissor(buffer, 0, 1, &scissor);
}

#ifdef __cplusplus
}
#endif