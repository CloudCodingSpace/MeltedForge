#include "mfpipeline.h"

#include "vk/backend.h"
#include "vk/pipeline.h"
#include "vk/common.h"
#include "vk/image.h"
#include "vk/buffer.h"

struct MFPipeline_s {
    VulkanBackend* backend;
    VulkanBackendCtx* ctx;
    VulkanPipeline pipeline;
    MFPipelineConfig info;

    VkDescriptorSet descSet[FRAMES_IN_FLIGHT];
    VkDescriptorSetLayout descLayout;
};

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineConfig* info) {
    MF_ASSERT(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_ASSERT(info == mfnull, mfGetLogger(), "The pipeline info handle provided shouldn't be null!");

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

    u32 resourceDescCount = info->imgCount + info->buffCount;
    MFResourceDesc* resourceDescs = MF_ALLOCMEM(MFResourceDesc, sizeof(MFResourceDesc) * resourceDescCount);
    
    {
        for(u32 i = 0; i < info->imgCount; i++) {
            resourceDescs[i] = mfGetGpuImageDescription(info->images[i]);
        }
        for(u32 i = info->imgCount; i < resourceDescCount; i++) {
            resourceDescs[i] = mfGetGpuBufferDescription(info->buffers[i - info->imgCount]);
        }
    }

    // Descriptor Layout
    {
        VkDescriptorSetLayoutBinding* layBindings = MF_ALLOCMEM(VkDescriptorSetLayoutBinding, sizeof(VkDescriptorSetLayoutBinding) * resourceDescCount);
        for(u32 i = 0; i < resourceDescCount; i++) {
            layBindings[i].binding = resourceDescs[i].binding;
            layBindings[i].descriptorType = (VkDescriptorType)((int)resourceDescs[i].descriptorType);
            layBindings[i].descriptorCount = resourceDescs[i].descriptorCount;
            layBindings[i].stageFlags = (VkShaderStageFlags)((int)resourceDescs[i].stageFlags);
        }

        VkDescriptorSetLayoutCreateInfo layInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = resourceDescCount,
            .pBindings = layBindings
        };

        VK_CHECK(vkCreateDescriptorSetLayout(pipeline->ctx->device, &layInfo, pipeline->ctx->allocator, &pipeline->descLayout));
        MF_FREEMEM(layBindings);
    }

    VulkanPipelineInfo binfo = {
        .vertPath = info->vertPath,
        .fragPath = info->fragPath,
        .pass = pipeline->backend->pass, // TODO: Change this if we support more than one renderpass
        .hasDepth = info->hasDepth,
        .extent = (VkExtent2D) {
            info->extent.x,
            info->extent.y
        },
        .attribDescsCount = info->attribDescsCount,
        .attribDescs = attribs,
        .bindingDescsCount = info->bindingDescsCount,
        .bindingDescs = bindings,
        .setLayoutCount = 1,
        .setLayouts = &pipeline->descLayout
    };

    VulkanPipelineCreate(pipeline->ctx, &pipeline->pipeline, binfo);

    MF_FREEMEM(bindings);
    MF_FREEMEM(attribs);

    // Descriptor Set
    {
        VkDescriptorSetAllocateInfo setInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pipeline->ctx->descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &pipeline->descLayout
        };

        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VK_CHECK(vkAllocateDescriptorSets(pipeline->ctx->device, &setInfo, &pipeline->descSet[i]));
        }
    }
    // Updating Descriptor Sets
    {
        u32 count = info->imgCount + info->buffCount;

        VkWriteDescriptorSet* writes = MF_ALLOCMEM(VkWriteDescriptorSet, sizeof(VkWriteDescriptorSet) * count);
        VkDescriptorImageInfo* imgInfos = MF_ALLOCMEM(VkDescriptorImageInfo, sizeof(VkDescriptorImageInfo) * info->imgCount);
        VkDescriptorBufferInfo* buffInfos = MF_ALLOCMEM(VkDescriptorBufferInfo, sizeof(VkDescriptorBufferInfo) * info->buffCount);

        for(u32 i = 0; i < info->imgCount; i++) {
            imgInfos[i] = (VkDescriptorImageInfo){
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = mfGetGpuImageBackend(info->images[i]).view,
                .sampler = mfGetGpuImageBackend(info->images[i]).sampler
            };

            writes[i] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .dstBinding = resourceDescs[i].binding,
                .dstArrayElement = 0,
                .pImageInfo = &imgInfos[i]
            };
        }
        for(u32 i = info->imgCount; i < count; i++) {
            u32 j = i - info->imgCount;
            buffInfos[j] = (VkDescriptorBufferInfo){
                .buffer = mfGetGpuBufferBackend(info->buffers[j])->handle,
                .offset = 0, // TODO: Make it configurable if required
                .range = mfGetGpuBufferBackend(info->buffers[j])->size
            };

            writes[i] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .dstBinding = resourceDescs[i].binding,
                .dstArrayElement = 0,
                .pBufferInfo = &buffInfos[j]
            };
        }

        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            for(u32 j = 0; j < count; j++) {
                writes[j].dstSet = pipeline->descSet[i];
            }

            vkUpdateDescriptorSets(pipeline->ctx->device, count, writes, 0, NULL);
        }

        MF_FREEMEM(writes);
        MF_FREEMEM(buffInfos);
        MF_FREEMEM(imgInfos);
    }

    MF_FREEMEM(resourceDescs);
}

void mfPipelineDestroy(MFPipeline* pipeline) {
    MF_ASSERT(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");

    vkDestroyDescriptorSetLayout(pipeline->ctx->device, pipeline->descLayout, pipeline->ctx->allocator);
    vkFreeDescriptorSets(pipeline->ctx->device, pipeline->ctx->descPool, FRAMES_IN_FLIGHT, pipeline->descSet);
    VulkanPipelineDestroy(pipeline->ctx, &pipeline->pipeline);
    
    MF_SETMEM(pipeline, 0, sizeof(MFPipeline));
}

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor) {
    MF_ASSERT(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");
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

    vkCmdBindDescriptorSets(pipeline->backend->cmdBuffers[pipeline->backend->crntFrmIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline.layout, 0, 1, &pipeline->descSet[pipeline->backend->crntFrmIdx], 0, mfnull);
    VulkanPipelineBind(&pipeline->pipeline, v, s, pipeline->backend->cmdBuffers[pipeline->backend->crntFrmIdx]);
}

size_t mfPipelineGetSizeInBytes() {
    return sizeof(MFPipeline);
}