#include "mfpipeline.h"

#include "vk/backend.h"
#include "vk/pipeline.h"
#include "vk/common.h"
#include "vk/image.h"
#include "vk/buffer.h"
#include "vk/render_target.h"

#include <vulkan/vk_enum_string_helper.h>

struct MFPipeline_s {
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    VulkanPipeline pipeline;
    b8 init;
};

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineConfig* info) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(pipeline->init, mfGetLogger(), "The pipeline is already initialised!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(info == mfnull, mfGetLogger(), "The pipeline info handle provided shouldn't be null!");

    pipeline->ctx = &((VulkanBackend*)mfRendererGetBackend(renderer))->ctx;
    pipeline->backend = (VulkanBackend*)mfRendererGetBackend(renderer);

    VkVertexInputBindingDescription* bindings = MF_ALLOCMEM(VkVertexInputBindingDescription, sizeof(VkVertexInputBindingDescription) * info->bindingsCount);
    VkVertexInputAttributeDescription* attribs = MF_ALLOCMEM(VkVertexInputAttributeDescription, sizeof(VkVertexInputAttributeDescription) * info->attributesCount);

    for (u32 i = 0; i < info->bindingsCount; i++) {
        bindings[i].binding = info->bindings[i].binding;
        bindings[i].inputRate = (VkVertexInputRate)((int)info->bindings[i].rate);
        bindings[i].stride = info->bindings[i].stride;
    }

    for (u32 i = 0; i < info->attributesCount; i++) {
        attribs[i].binding = info->attributes[i].binding;
        attribs[i].format = (VkFormat)((int)info->attributes[i].format);
        attribs[i].location = info->attributes[i].location;
        attribs[i].offset = info->attributes[i].offset;
    }

    VkDescriptorSetLayout* setLayouts = MF_ALLOCMEM(VkDescriptorSetLayout, sizeof(VkDescriptorSetLayout) * info->resourceLayoutCount);
    for(u32 i = 0; i < info->resourceLayoutCount; i++) {
        setLayouts[i] = mfResourceSetLayoutGetBackend(info->resourceLayouts[i]);
    }

    // Push constant size check
    {
        u32 totalSize = 0;
        for(u64 i = 0; i < info->pushConstRangeCount; i++) {
            totalSize += info->pushConstRanges[i].size;
        }
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(pipeline->backend->ctx.physicalDevice, &properties);

        MF_PANIC_IF(properties.limits.maxPushConstantsSize < totalSize, mfGetLogger(), "The total push constant size of the pipeline is greater than the GPU's limits!");
    }

    VkPushConstantRange* ranges = MF_ALLOCMEM(VkPushConstantRange, sizeof(VkPushConstantRange) * info->pushConstRangeCount);
    for(u64 i = 0; i < info->pushConstRangeCount; i++) {
        ranges[i].offset = info->pushConstRanges[i].offset;
        ranges[i].size = info->pushConstRanges[i].size;
        ranges[i].stageFlags = (VkShaderStageFlags)((int)info->pushConstRanges[i].stage);
    }

    VulkanPipelineInfo binfo = {
        .vertPath = info->vertPath,
        .fragPath = info->fragPath,
        .renderpass = mfRendererGetRenderPass(renderer),
        .depthCompareOp = (VkCompareOp)(int)info->depthCompareOp,
        .hasDepth = info->hasDepth,
        .extent = (VkExtent2D) { info->extent.x, info->extent.y },
        .attributesCount = info->attributesCount,
        .attributes = attribs,
        .bindingsCount = info->bindingsCount,
        .bindings = bindings,
        .setLayoutCount = info->resourceLayoutCount,
        .setLayouts = setLayouts,
        .pushConstRangesCount = info->pushConstRangeCount,
        .pushConstRanges = ranges,
        .cache = pipeline->backend->pipelineCache
    };

    if(info->renderTarget != mfnull) {
        binfo.renderpass = info->renderTarget->renderPass;
    }

    VulkanPipelineCreate(pipeline->ctx, &pipeline->pipeline, &binfo);

    MF_FREEMEM(ranges);
    MF_FREEMEM(setLayouts);
    MF_FREEMEM(bindings);
    MF_FREEMEM(attribs);

    pipeline->init = true;
}

void mfPipelineDestroy(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");
    
    VulkanPipelineDestroy(pipeline->ctx, &pipeline->pipeline);
    
    MF_SETMEM(pipeline, 0, sizeof(MFPipeline));
}

void mfPipelinePushConstant(MFPipeline* pipeline, MFShaderStage shaderStage, u32 offset, u32 size, void* data) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");

    VkCommandBuffer commandBuffer = pipeline->backend->commandBuffers[pipeline->backend->frameIndex];
    if(pipeline->backend->renderTarget != mfnull) {
        commandBuffer = pipeline->backend->renderTarget->commandBuffers[pipeline->backend->frameIndex];
    }

    vkCmdPushConstants(commandBuffer, pipeline->pipeline.layout, (VkShaderStageFlags)((int)shaderStage), offset, size, data);
}

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_PANIC_IF(!pipeline->init, mfGetLogger(), "The pipeline isn't initialised!");

    vp.y = vp.height;
    vp.height *= -1.0f;
    
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
    if(pipeline->backend->renderTarget != mfnull) {
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
