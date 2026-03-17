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

    VkVertexInputBindingDescription* bindings = MF_ALLOCMEM(VkVertexInputBindingDescription, sizeof(VkVertexInputBindingDescription) * info->bindingDescsCount);
    VkVertexInputAttributeDescription* attribs = MF_ALLOCMEM(VkVertexInputAttributeDescription, sizeof(VkVertexInputAttributeDescription) * info->attribDescsCount);

    for (u32 i = 0; i < info->bindingDescsCount; i++) {
        bindings[i].binding = info->bindingDescs[i].binding;
        bindings[i].inputRate = (VkVertexInputRate)((int)info->bindingDescs[i].rate);
        bindings[i].stride = info->bindingDescs[i].stride;
    }

    for (u32 i = 0; i < info->attribDescsCount; i++) {
        attribs[i].binding = info->attribDescs[i].binding;
        attribs[i].format = (VkFormat)((int)info->attribDescs[i].format);
        attribs[i].location = info->attribDescs[i].location;
        attribs[i].offset = info->attribDescs[i].offset;
    }

    VkDescriptorSetLayout* setLayouts = MF_ALLOCMEM(VkDescriptorSetLayout, sizeof(VkDescriptorSetLayout) * info->resLayCount);
    for(u32 i = 0; i < info->resLayCount; i++) {
        setLayouts[i] = mfGetResourceSetLayoutBackend(info->resLayouts[i]);
    }

    VulkanPipelineInfo binfo = {
        .vertPath = info->vertPath,
        .fragPath = info->fragPath,
        .pass = info->pass,
        .depthCompareOp = (VkCompareOp)(int)info->depthCompareOp,
        .hasDepth = info->hasDepth,
        .extent = (VkExtent2D) { info->extent.x, info->extent.y },
        .attribDescsCount = info->attribDescsCount,
        .attribDescs = attribs,
        .bindingDescsCount = info->bindingDescsCount,
        .bindingDescs = bindings,
        .setLayoutCount = info->resLayCount,
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
    
    VkCommandBuffer buff = pipeline->backend->cmdBuffers[pipeline->backend->crntFrmIdx];
    if(pipeline->backend->rt != mfnull) {
        buff = pipeline->backend->rt->buffs[pipeline->backend->crntFrmIdx];
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
