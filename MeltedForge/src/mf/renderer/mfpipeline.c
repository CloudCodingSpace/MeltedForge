#ifdef __cplusplus
extern "C" {
#endif

#include "mfpipeline.h"

#include "vk/backend.h"
#include "vk/pipeline.h"
#include "vk/common.h"
#include "vk/image.h"
#include "vk/buffer.h"
#include "vk/render_target.h"
#include "vk/command_buffer.h"

#include <vulkan/vk_enum_string_helper.h>

struct MFPipeline_s {
    MFPipelineConfig config;
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    VulkanPipeline pipeline;
    VkSemaphore semas[FRAMES_IN_FLIGHT];
    bool init;
};

MFPipeline* mfPipelineCreate(MFRenderer* renderer, MFPipelineConfig info) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF((info.type < 0) || (info.type >= MF_PIPELINE_TYPE_COUNT), mfGetLogger(), "The pipeline type provided is invalid!");

    MFPipeline* pipeline = MF_ALLOCMEM(MFPipeline, sizeof(MFPipeline));

    pipeline->config = info;
    pipeline->ctx = &((VulkanBackend*)mfRendererGetBackend(renderer))->ctx;
    pipeline->backend = (VulkanBackend*)mfRendererGetBackend(renderer);

    VkSemaphoreCreateInfo semaInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(pipeline->ctx->device, &semaInfo, pipeline->ctx->allocator, &pipeline->semas[i]));
    }

    VkVertexInputBindingDescription* bindings = mfnull;
    VkVertexInputAttributeDescription* attribs = mfnull;

    VkDescriptorSetLayout* setLayouts = MF_ALLOCMEM(VkDescriptorSetLayout, sizeof(VkDescriptorSetLayout) * info.resourceLayoutCount);
    for(u32 i = 0; i < info.resourceLayoutCount; i++) {
        setLayouts[i] = mfResourceSetLayoutGetBackend( info.resourceLayouts[i]);
    }

    // Push constant size check
    {
        u32 totalSize = 0;
        for(u64 i = 0; i < info.pushConstRangeCount; i++) {
            totalSize += info.pushConstRanges[i].size;
        }
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(pipeline->backend->ctx.physicalDevice, &properties);

        MF_PANIC_IF(properties.limits.maxPushConstantsSize < totalSize, mfGetLogger(), "The total push constant size of the pipeline is greater than the GPU's limits!");
    }

    VkPushConstantRange* ranges = MF_ALLOCMEM(VkPushConstantRange, sizeof(VkPushConstantRange) * info.pushConstRangeCount);
    for(u64 i = 0; i < info.pushConstRangeCount; i++) {
        ranges[i].offset = info.pushConstRanges[i].offset;
        ranges[i].size = info.pushConstRanges[i].size;
        ranges[i].stageFlags = (VkShaderStageFlags)((int) info.pushConstRanges[i].stage);
    }

    VulkanPipelineInfo binfo = {
        .setLayoutCount = info.resourceLayoutCount,
        .setLayouts = setLayouts,
        .pushConstRangesCount = info.pushConstRangeCount,
        .pushConstRanges = ranges,
        .cache = pipeline->backend->pipelineCache,
        .type = (VulkanPipelineType)(u32)info.type
    };
    
    if(info.type == MF_PIPELINE_TYPE_GRAPHICS) {
        bindings = MF_ALLOCMEM(VkVertexInputBindingDescription, sizeof(VkVertexInputBindingDescription) * info.graphicsConfig.bindingsCount);
        attribs = MF_ALLOCMEM(VkVertexInputAttributeDescription, sizeof(VkVertexInputAttributeDescription) * info.graphicsConfig.attributesCount);
        
        for (u32 i = 0; i < info.graphicsConfig.bindingsCount; i++) {
            bindings[i].binding = info.graphicsConfig.bindings[i].binding;
            bindings[i].inputRate = (VkVertexInputRate)((int) info.graphicsConfig.bindings[i].rate);
            bindings[i].stride = info.graphicsConfig.bindings[i].stride;
        }
        
        for (u32 i = 0; i < info.graphicsConfig.attributesCount; i++) {
            attribs[i].binding = info.graphicsConfig.attributes[i].binding;
            attribs[i].format = (VkFormat)((int) info.graphicsConfig.attributes[i].format);
            attribs[i].location = info.graphicsConfig.attributes[i].location;
            attribs[i].offset = info.graphicsConfig.attributes[i].offset;
        }

        binfo.ginfo = (VulkanGPipelineInfo) {
            .vertPath = info.graphicsConfig.vertPath,
            .fragPath = info.graphicsConfig.fragPath,
            .renderpass = mfRendererGetRenderPass(renderer),
            .depthCompareOp = (VkCompareOp)(int)info.graphicsConfig.depthCompareOp,
            .hasDepth = info.graphicsConfig.hasDepth,
            .extent = (VkExtent2D) { info.graphicsConfig.extent.x, info.graphicsConfig.extent.y },
            .attributesCount = info.graphicsConfig.attributesCount,
            .attributes = attribs,
            .bindingsCount = info.graphicsConfig.bindingsCount,
            .bindings = bindings,
            .cullMode = (VkCullModeFlags)(int) info.graphicsConfig.cullMode,
            .samples = pipeline->ctx->samples
        };

        if(info.graphicsConfig.renderTarget != mfnull) {
            VulkanRenderTarget* renderTarget = mfRenderTargetGetBackend(info.graphicsConfig.renderTarget);
            binfo.ginfo.renderpass = renderTarget->renderPass.handle;
        }
    }
    else if(info.type == MF_PIPELINE_TYPE_COMPUTE) {
        binfo.cinfo = (VulkanCPipelineInfo) {
            .path = info.computeConfig.filePath
        };
    }
    
    VulkanPipelineCreate(pipeline->ctx, &pipeline->pipeline, &binfo);
    
    MF_FREEMEM(ranges);
    MF_FREEMEM(setLayouts);
    if(info.type == MF_PIPELINE_TYPE_GRAPHICS) {
        MF_FREEMEM(bindings);
        MF_FREEMEM(attribs);
    }

    pipeline->init = true;
    return pipeline;
}

void mfPipelineDestroy(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");
    
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(pipeline->ctx->device, pipeline->semas[i], pipeline->ctx->allocator);
    }

    VulkanPipelineDestroy(pipeline->ctx, &pipeline->pipeline);
    
    MF_SETMEM(pipeline, 0, sizeof(MFPipeline));
    MF_FREEMEM(pipeline);
}

void mfPipelinePrepareComputeDispatch(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");

    if(pipeline->config.type != MF_PIPELINE_TYPE_COMPUTE) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "Can't begin dispatch for non-compute pipelines!");
        return;
    }

    if(pipeline->backend->ctx.dispatchBegun) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "Can't start dispatch when one is already going on!");
        return;
    }

    VkCommandBuffer buff = pipeline->backend->computeCmdBuffers[pipeline->backend->frameIndex];
    VK_CHECK(vkResetCommandBuffer(buff, 0));
    VulkanCommandBufferBegin(buff, true);

    vkCmdBindPipeline(buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline.pipeline);

    pipeline->backend->ctx.dispatchBegun = true;
}

void mfPipelineComputeDispatch(MFPipeline* pipeline, u32 workgroupSizeX, u32 workgroupSizeY) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");

    if(pipeline->config.type != MF_PIPELINE_TYPE_COMPUTE) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "Can't dispatch for non-compute pipelines!");
        return;
    }

    if(!pipeline->backend->ctx.dispatchBegun) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "Can't dispatch pipeline when one hasn't begun yet!");
        return;
    }

    VkCommandBuffer buff = pipeline->backend->computeCmdBuffers[pipeline->backend->frameIndex];
    vkCmdDispatch(buff, workgroupSizeX, workgroupSizeY, 1);
    VulkanCommandBufferEnd(buff);

    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &buff,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &pipeline->semas[pipeline->backend->frameIndex]
    };
    VK_CHECK(vkQueueSubmit(pipeline->ctx->queueData.computeQueue, 1, &info, VK_NULL_HANDLE));

    // TODO: Make this search faster if required
    bool exists = false;
    for(u64 i = 0; i < pipeline->backend->waitSemas.len; i++) {
        if(mfArrayGetElement(pipeline->backend->waitStages, VkSemaphore, i) == pipeline->semas[pipeline->backend->frameIndex])
            exists = true;
    }
    if(!exists) {
        mfArrayAddElement(&pipeline->backend->waitSemas, VkSemaphore, pipeline->semas[pipeline->backend->frameIndex]);
        mfArrayAddElement(&pipeline->backend->waitStages, VkPipelineStageFlags, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    pipeline->backend->ctx.dispatchBegun = false;
}

void mfPipelinePushConstant(MFPipeline* pipeline, MFShaderStage shaderStage, u32 offset, u32 size, void* data) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");

    VkCommandBuffer commandBuffer = pipeline->backend->commandBuffers[pipeline->backend->frameIndex];
    if(pipeline->config.type == MF_PIPELINE_TYPE_COMPUTE && pipeline->backend->ctx.dispatchBegun) {
        commandBuffer = pipeline->backend->computeCmdBuffers[pipeline->backend->frameIndex];
    }
    else if(pipeline->backend->renderTarget != mfnull) {
        commandBuffer = pipeline->backend->renderTarget->commandBuffers[pipeline->backend->frameIndex];
    }

    vkCmdPushConstants(commandBuffer, pipeline->pipeline.layout, (VkShaderStageFlags)((int)shaderStage), offset, size, data);
}

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");
    
    VkViewport v = {
        .x = vp.x,
        .y = vp.y,
        .width = vp.width,
        .height = vp.height,
        .maxDepth = vp.maxDepth,
        .minDepth = vp.minDepth
    };
    
    VkRect2D s = {
        .extent = (VkExtent2D){scissor.extentX, scissor.extentY},
        .offset = (VkOffset2D){scissor.offsetX, scissor.offsetY}
    };
    
    VkCommandBuffer commandBuffer = pipeline->backend->commandBuffers[pipeline->backend->frameIndex];
    if(pipeline->config.type == MF_PIPELINE_TYPE_COMPUTE && pipeline->backend->ctx.dispatchBegun) {
        commandBuffer = pipeline->backend->computeCmdBuffers[pipeline->backend->frameIndex];
    }
    else if(pipeline->backend->renderTarget != mfnull) {
        commandBuffer = pipeline->backend->renderTarget->commandBuffers[pipeline->backend->frameIndex];
    }
    
    VulkanPipelineBind(&pipeline->pipeline, v, s, commandBuffer);
}

void* mfPipelineGetLayoutBackend(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");
    
    return pipeline->pipeline.layout;
}

void* mfPipelineGetBackend(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");

    return &pipeline->pipeline;
}

size_t mfPipelineGetSizeInBytes(void) {
    return sizeof(MFPipeline);
}

#ifdef __cplusplus
}
#endif