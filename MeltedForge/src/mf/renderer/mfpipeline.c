#include "mfpipeline.h"

#include "vk/backend.h"
#include "vk/pipeline.h"

struct MFPipeline_s {
    VulkanBackendCtx* ctx;
    VulkanPipeline pipeline;
    MFPipelineInfo info;
};

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineInfo* info) {
    MF_ASSERT(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_ASSERT(info == mfnull, mfGetLogger(), "The pipeline info handle provided shouldn't be null!");

    pipeline->ctx = &((VulkanBackend*)mfRendererGetBackend(renderer))->ctx;

    VulkanPipelineInfo binfo = {
        .vertPath = info->vertPath,
        .fragPath = info->fragPath,
        .pass = ((VulkanBackend*)mfRendererGetBackend(renderer))->pass, // TODO: Change this if we support more than one renderpass
        .hasDepth = info->hasDepth,
        .extent = (VkExtent2D) {
            info->extent.x,
            info->extent.y
        },
        .attribDescs = 0, // TODO: Finish this later
        .bindingDescs = 0 // TODO: Finish this later
    };

    VulkanPipelineCreate(pipeline->ctx, &pipeline->pipeline, binfo);
}

void mfPipelineDestroy(MFPipeline* pipeline) {
    MF_ASSERT(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");

    VulkanPipelineDestroy(pipeline->ctx, &pipeline->pipeline);

    MF_SETMEM(pipeline, 0, sizeof(MFPipeline));
}

size_t mfPipelineGetSizeInBytes() {
    return sizeof(MFPipeline);
}