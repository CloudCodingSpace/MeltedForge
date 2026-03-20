#include "mfpipeline.h"

#include "vk/backend.h"
#include "vk/pipeline.h"
#include "vk/common.h"
#include "vk/image.h"
#include "vk/buffer.h"
#include "vk/render_target.h"

struct MFPipeline_s {
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    VulkanPipeline pipeline;
};

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineConfig* info) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
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

    VulkanPipelineInfo binfo = {
        .vertPath = info->vertPath,
        .fragPath = info->fragPath,
        .renderpass = info->renderpass,
        .depthCompareOp = (VkCompareOp)(int)info->depthCompareOp,
        .hasDepth = info->hasDepth,
        .extent = (VkExtent2D) { info->extent.x, info->extent.y },
        .attributesCount = info->attributesCount,
        .attributes = attribs,
        .bindingsCount = info->bindingsCount,
        .bindings = bindings,
        .setLayoutCount = info->resourceLayoutCount,
        .setLayouts = setLayouts
    };

    VulkanPipelineCreate(pipeline->ctx, &pipeline->pipeline, &binfo);

    MF_FREEMEM(setLayouts);
    MF_FREEMEM(bindings);
    MF_FREEMEM(attribs);
}

void mfPipelineDestroy(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");

    VulkanPipelineDestroy(pipeline->ctx, &pipeline->pipeline);

    MF_SETMEM(pipeline, 0, sizeof(MFPipeline));
}

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
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
    
    VkCommandBuffer buff = pipeline->backend->cmdBuffers[pipeline->backend->frameIndex];
    if(pipeline->backend->rt != mfnull) {
        buff = pipeline->backend->rt->commandBuffers[pipeline->backend->frameIndex];
    }
    
    VulkanPipelineBind(&pipeline->pipeline, v, s, buff);
}

void* mfPipelineGetLayoutBackend(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    
    return pipeline->pipeline.layout;
}

void* mfPipelineGetBackend(MFPipeline* pipeline) {
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");

    return pipeline->pipeline.pipeline;
}

size_t mfPipelineGetSizeInBytes(void) {
    return sizeof(MFPipeline);
}
