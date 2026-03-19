#include "mfgpu_res.h"

#include <vulkan/vulkan.h>

#include "mfgpubuffer.h"
#include "mfgpuimage.h"
#include "mfpipeline.h"

#include "vk/backend.h"
#include "vk/buffer.h"
#include "vk/image.h"
#include "vk/render_target.h"
#include "vk/pipeline.h"

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    MFRenderer* renderer;
    MFArray resDescs;
    u64 imageCount, bufferCount;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
};

void mfResourceSetLayoutCreate(MFResourceSetLayout* layout, u64 resDescLen, MFResourceDesc* resDescs, MFRenderer* renderer) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The provided renderer handle shouldn't be null!");
    MF_PANIC_IF(resDescLen == 0, mfGetLogger(), "The provided resource description count shouldn't be 0!");
    MF_PANIC_IF(resDescs == mfnull, mfGetLogger(), "The provided resource descriptions shouldn't be null!");
    
    layout->renderer = renderer;
    layout->resDescs = mfArrayCreate(mfGetLogger(), resDescLen, sizeof(MFResourceDesc));
    layout->resDescs.len = resDescLen;

    for(u64 i = 0; i < resDescLen; i++) {
        mfArrayGet(layout->resDescs, MFResourceDesc, i) = resDescs[i];
    }

    // TODO: Support more shader res type!
    u64 imageCount = 0, bufferCount = 0;
    for(u64 i = 0; i < resDescLen; i++) {
        if(resDescs[i].descriptorType == MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER) {
            imageCount++;
        }
        if(resDescs[i].descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER) {
            bufferCount++;;
        }
    }

    layout->imageCount = imageCount;
    layout->bufferCount = bufferCount;

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    // Descriptor pool
    {
        u32 count = 0;
        VkDescriptorPoolSize sizes[2] = {0};

        if(imageCount != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * FRAMES_IN_FLIGHT };
        }
        if(bufferCount != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferCount * FRAMES_IN_FLIGHT };
        }

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1000, //! FIXME: INSTEAD OF ASSUMING 1000, CREATE A DYNAMIC DESCRIPTOR POOL ALLOCATOR
            .poolSizeCount = count,
            .pPoolSizes = sizes,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        };

        VK_CHECK(vkCreateDescriptorPool(ctx->device, &info, ctx->allocator, &layout->pool));
    }

    // Descriptor layout
    {
        VkDescriptorSetLayoutBinding* layBindings = MF_ALLOCMEM(VkDescriptorSetLayoutBinding, sizeof(VkDescriptorSetLayoutBinding) * resDescLen);
        u32 uniqueBindingCount = 0;

        for(u32 i = 0; i < resDescLen; i++) {
            bool bindingExists = false;
            for(u32 j = 0; j < uniqueBindingCount; j++) {
                if(layBindings[j].binding == resDescs[i].binding) {
                    bindingExists = true;
                    break;
                }
            }

            if(!bindingExists) {
                layBindings[uniqueBindingCount].binding = resDescs[i].binding;
                layBindings[uniqueBindingCount].descriptorType = (VkDescriptorType)((int)resDescs[i].descriptorType);
                layBindings[uniqueBindingCount].descriptorCount = resDescs[i].descriptorCount;
                layBindings[uniqueBindingCount].stageFlags = (VkShaderStageFlags)((int)resDescs[i].stageFlags);
                uniqueBindingCount++;
            }
        }

        MF_DO_IF(uniqueBindingCount != resDescLen, {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The bindings of each resource description in a layout must be unique! But the provided descriptions aren't unique!");
        });

        VkDescriptorSetLayoutCreateInfo layInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = uniqueBindingCount,
            .pBindings = layBindings
        };

        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device, &layInfo, ctx->allocator, &layout->layout));
        MF_FREEMEM(layBindings);
    }
}

void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(layout->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    vkDestroyDescriptorPool(ctx->device, layout->pool, ctx->allocator);
    vkDestroyDescriptorSetLayout(ctx->device, layout->layout, ctx->allocator);

    mfArrayDestroy(&layout->resDescs, mfGetLogger());

    MF_SETMEM(layout, 0, sizeof(MFResourceSetLayout));
}

void mfResourceSetCreate(MFResourceSet* set, MFResourceSetLayout* layout, MFRenderer* renderer) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The resource set layout handle provided shoudln't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    set->layout = layout;
    set->renderer = renderer;

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = layout->pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout->layout
    };

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
        VK_CHECK(vkAllocateDescriptorSets(ctx->device, &info, &set->sets[i]));
}

void mfResourceSetDestroy(MFResourceSet* set) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VK_CHECK(vkFreeDescriptorSets(ctx->device, set->layout->pool, FRAMES_IN_FLIGHT, set->sets));

    MF_SETMEM(set, 0, sizeof(MFResourceSet));
}

void mfResourceSetBind(MFResourceSet* set, MFPipeline* pipeline) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VkCommandBuffer buff = backend->cmdBuffers[backend->crntFrmIdx];
    if(backend->rt != mfnull) {
        buff = backend->rt->buffs[backend->crntFrmIdx];
    }

    //! FIXME: ASSUMING THE BIND POINT TO BE GRAPHICS, CHANGE THIS IF MORE TYPE OF PIPELINES ARE INTRODUCED
    vkCmdBindDescriptorSets(buff, VK_PIPELINE_BIND_POINT_GRAPHICS, mfPipelineGetLayoutBackend(pipeline), 
                                    0, 1, &set->sets[backend->crntFrmIdx], 
                                    0, mfnull);
}

void mfResourceSetUpdate(MFResourceSet* set, MFArray* images, MFArray* buffers) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");

    u64 imgCount = 0, buffCount = 0;
    if(images != mfnull) {
        imgCount = images->len;
    }
    if(buffers != mfnull) {
        buffCount = buffers->len;
    }

    MF_PANIC_IF(imgCount != set->layout->imageCount, mfGetLogger(), "The image array doesn't follow the resource set layout!");
    MF_PANIC_IF(buffCount != set->layout->bufferCount, mfGetLogger(), "The buffer array doesn't follow the resource set layout!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    u64 count = set->layout->imageCount + set->layout->bufferCount;
    VkWriteDescriptorSet* writes = MF_ALLOCMEM(VkWriteDescriptorSet, sizeof(VkWriteDescriptorSet) * count);
    VkDescriptorImageInfo* imgInfos = MF_ALLOCMEM(VkDescriptorImageInfo, sizeof(VkDescriptorImageInfo) * set->layout->imageCount);
    VkDescriptorBufferInfo* buffInfos = MF_ALLOCMEM(VkDescriptorBufferInfo, sizeof(VkDescriptorBufferInfo) * set->layout->bufferCount);

    for(u64 i = 0; i < set->layout->imageCount; i++) {
        imgInfos[i] = (VkDescriptorImageInfo){
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = mfGpuImageGetBackend(mfArrayGet(*images, MFGpuImage*, i)).view,
            .sampler = mfGpuImageGetBackend(mfArrayGet(*images, MFGpuImage*, i)).sampler
        };
    }
    
    for (u32 frame = 0; frame < FRAMES_IN_FLIGHT; frame++) {
        u64 writeIdx = 0;

        // Images
        for (u64 i = 0; i < set->layout->imageCount; i++) {
            writes[writeIdx] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set->sets[frame],
                .dstBinding = mfGpuImageGetDescription(mfArrayGet(*images, MFGpuImage*, i)).binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &imgInfos[i]
            };
            writeIdx++;
        }

        // Buffers
        for (u64 i = 0; i < set->layout->bufferCount; i++) {
            MF_PANIC_IF(mfGpuBufferGetBackend(mfArrayGet(*buffers, MFGpuBuffer*, i))->type != VULKAN_BUFFER_TYPE_UBO, mfGetLogger(), 
                                        "The given buffer for resource set isn't of an uniform buffer!");

            buffInfos[i] = (VkDescriptorBufferInfo){
                .buffer = mfGpuBufferGetBackend(mfArrayGet(*buffers, MFGpuBuffer*, i))[frame].handle,
                .offset = 0,
                .range = mfGpuBufferGetBackend(mfArrayGet(*buffers, MFGpuBuffer*, i))[frame].size
            };

            writes[writeIdx] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set->sets[frame],
                .dstBinding = mfGpuBufferGetDescription(mfArrayGet(*buffers, MFGpuBuffer*, i)).binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &buffInfos[i]
            };

            writeIdx++;
        }

        vkUpdateDescriptorSets(ctx->device, writeIdx, writes, 0, NULL);
    }

    MF_FREEMEM(writes);
    MF_FREEMEM(buffInfos);
    MF_FREEMEM(imgInfos);
}

void* mfResourceSetLayoutGetBackend(MFResourceSetLayout* layout) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");
    
    return layout->layout;
}

size_t mfResourceSetLayoutGetSizeInBytes(void) {
    return sizeof(MFResourceSetLayout);
}
size_t mfResourceSetGetSizeInBytes(void) {
    return sizeof(MFResourceSet);
}