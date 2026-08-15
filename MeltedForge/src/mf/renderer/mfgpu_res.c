#ifdef __cplusplus
extern "C" {
#endif

#include "mfgpu_res.h"
#include "mfpipeline.h"
#include "mfgpubuffer.h"
#include "mfgpuimage.h"

#include "vk/render_target.h"
#include "vk/pipeline.h"
#include "vk/gpu_res.h"

MFResourceSetLayout* mfResourceSetLayoutCreate(u64 bindingLen, MFResourceSetBindings* bindings, MFRenderer* renderer) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The provided renderer handle shouldn't be null!");
    MF_PANIC_IF(bindingLen == 0, mfGetLogger(), "The provided resource binding count shouldn't be 0!");
    MF_PANIC_IF(bindings == mfnull, mfGetLogger(), "The provided resource bindings shouldn't be null!");

    MFResourceSetLayout* layout = MF_ALLOCMEM(MFResourceSetLayout, sizeof(MFResourceSetLayout));

    layout->renderer = renderer;
    layout->bindings = mfArrayCreate(bindingLen, sizeof(MFResourceSetBindings));
    layout->bindings.len = bindingLen;

    for(u64 i = 0; i < bindingLen; i++) {
        mfArraySetElement(layout->bindings, MFResourceSetBindings, i, bindings[i]);
    }

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    // Descriptor layout
    {
        VkDescriptorSetLayoutBinding* layBindings = MF_ALLOCMEM(VkDescriptorSetLayoutBinding, sizeof(VkDescriptorSetLayoutBinding) * bindingLen);
        u32 uniqueBindingCount = 0;

        for(u32 i = 0; i < bindingLen; i++) {
            bool bindingExists = false;
            for(u32 j = 0; j < uniqueBindingCount; j++) {
                if(layBindings[j].binding == bindings[i].binding) {
                    bindingExists = true;
                    break;
                }
            }

            if(!bindingExists) {
                layBindings[uniqueBindingCount].binding = bindings[i].binding;
                layBindings[uniqueBindingCount].descriptorType = (VkDescriptorType)((int)bindings[i].description.descriptorType);
                layBindings[uniqueBindingCount].descriptorCount = bindings[i].description.descriptorCount;
                layBindings[uniqueBindingCount].stageFlags = (VkShaderStageFlags)((int)bindings[i].description.stageFlags);
                uniqueBindingCount++;
            }
        }

        MF_DO_IF(uniqueBindingCount != bindingLen, {
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

    layout->init = true;
    return layout;
}

void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");
    MF_PANIC_IF(!layout->init, mfGetLogger(), "The resource set layout isn't initialised!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(layout->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    vkDestroyDescriptorSetLayout(ctx->device, layout->layout, ctx->allocator);

    mfArrayDestroy(&layout->bindings);

    MF_SETMEM(layout, 0, sizeof(MFResourceSetLayout));
    MF_FREEMEM(layout);
}

MFResourceSet* mfResourceSetCreate(MFResourceSetLayout* layout, MFRenderer* renderer) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The resource set layout handle provided shoudln't be null!");
    MF_PANIC_IF(!layout->init, mfGetLogger(), "The resource set layout isn't initialised!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFResourceSet* set = MF_ALLOCMEM(MFResourceSet, sizeof(MFResourceSet));

    set->layout = layout;
    set->renderer = renderer;

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    // Descriptor pool
    {
        u32 count = 0;
        VkDescriptorPoolSize sizes[4] = {0};

        // TODO: Support more shader res type!
        u64 indices[MF_RES_DESCRIPTION_TYPE_COUNT];
        MF_SETMEM(indices, 0, sizeof(indices));
        VulkanGpuResGetPoolSizesFromBindings(indices, layout->bindings.len, layout->bindings.data);

        if(indices[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, indices[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] * FRAMES_IN_FLIGHT };
        }
        if(indices[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, indices[MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER] * FRAMES_IN_FLIGHT };
        }
        if(indices[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indices[MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER] * FRAMES_IN_FLIGHT };
        }
        if(indices[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, indices[MF_RES_DESCRIPTION_TYPE_STORAGE_IMAGE] * FRAMES_IN_FLIGHT };
        }

        set->poolIdx = VulkanGpuResCreatePool(ctx, count, sizes, FRAMES_IN_FLIGHT);
    }

    VulkanGpuResDescriptorPool* pool = &mfArrayGetElement(ctx->descriptorPools, VulkanGpuResDescriptorPool, set->poolIdx);

    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool->pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout->layout
    };

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
        VK_CHECK(vkAllocateDescriptorSets(ctx->device, &info, &set->sets[i]));
    
    pool->allocatedSets += FRAMES_IN_FLIGHT;
    if(pool->allocatedSets == VULKAN_GPU_RES_MAX_DESCRIPTORS)
        pool->isFull = true;

    set->init = true;
    return set;
}

void mfResourceSetDestroy(MFResourceSet* set) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(!set->init, mfGetLogger(), "The resource set isn't initialised!");

    if(set->layout == mfnull) {
        return;
    } else if (!set->layout->init) {
        return;
    }
    
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;
    VulkanGpuResDescriptorPool* pool = &mfArrayGetElement(ctx->descriptorPools, VulkanGpuResDescriptorPool, set->poolIdx);

    VK_CHECK(vkFreeDescriptorSets(ctx->device, pool->pool, FRAMES_IN_FLIGHT, set->sets));

    pool->allocatedSets -= FRAMES_IN_FLIGHT;
    pool->isFull = false;

    MF_SETMEM(set, 0, sizeof(MFResourceSet));
    MF_FREEMEM(set);
}

void mfResourceSetsBind(u32 firstSetIndex, u64 setCount, MFResourceSet** sets, struct MFPipeline_s* pipeline) {
    MF_PANIC_IF(setCount == 0, mfGetLogger(), "The resource set array length provided shouldn't be 0!");
    MF_PANIC_IF(sets == mfnull, mfGetLogger(), "The resource set array provided shouldn't be null!");
    MF_PANIC_IF(pipeline == mfnull, mfGetLogger(), "The pipeline handle provided shouldn't be null!");

    for(u64 i = 0; i < setCount; i++) {
        MF_PANIC_IF(sets[i] == mfnull, mfGetLogger(), "The resource set in the array shouldn't be null!");
        MF_PANIC_IF(!sets[i]->init, mfGetLogger(), "The resource set isn't initialised!");
    }

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(sets[0]->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VulkanPipeline* pipelineBackend = (VulkanPipeline*)mfPipelineGetBackend(pipeline);

    VkCommandBuffer buff = backend->commandBuffers[backend->frameIndex];
    if(pipelineBackend->info.type == VULKAN_PIPELINE_TYPE_COMPUTE && backend->ctx.dispatchBegun) {
        buff = backend->computeCmdBuffers[backend->frameIndex];
    }
    else if(backend->renderTarget != mfnull) {
        buff = backend->renderTarget->commandBuffers[backend->frameIndex];
    }

    for(u64 i = 0; i < setCount; i++) {
        mfArrayAddElement(&backend->descSetBindingPool, VkDescriptorSet, sets[i]->sets[backend->frameIndex]);
    }

    vkCmdBindDescriptorSets(buff, pipelineBackend->bindPoint, pipelineBackend->layout, 
                                    firstSetIndex, setCount, (VkDescriptorSet*)backend->descSetBindingPool.data, 
                                    0, mfnull);

    mfArrayReset(&backend->descSetBindingPool);
}

void mfResourceSetUpdate(MFResourceSet* set, u32 imageCount, MFGpuImage** images, u32 bufferCount, MFGpuBuffer** buffers) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(!set->init, mfGetLogger(), "The resource set isn't initialised!");
    MF_PANIC_IF(imageCount && (images == mfnull), mfGetLogger(), "The image count for set update is more than 0 then the image array shouldn't be null!");
    MF_PANIC_IF(bufferCount && (buffers == mfnull), mfGetLogger(), "The buffer count for set update is more than 0 then the buffer array shouldn't be null!");

    u64 bindingImgCount = 0, bindingBuffCount = 0;
    {
        VkDescriptorPoolSize sizes[4] = {0};

        // TODO: Support more shader res type!
        u64 indices[MF_RES_DESCRIPTION_TYPE_COUNT];
        MF_SETMEM(indices, 0, sizeof(indices));
        VulkanGpuResGetPoolSizesFromBindings(indices, set->layout->bindings.len, set->layout->bindings.data);
    
        if(indices[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] != 0) {
            bindingImgCount += indices[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER];
        }
        if(indices[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] != 0) {
            bindingImgCount += indices[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE];
        }
        if(indices[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] != 0) {
            bindingBuffCount += indices[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER];
        }
        if(indices[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] != 0) {
            bindingBuffCount += indices[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER];
        }
    }

    MF_PANIC_IF(imageCount != bindingImgCount, mfGetLogger(), "The image array doesn't follow the resource set layout!");
    MF_PANIC_IF(bufferCount != bindingBuffCount, mfGetLogger(), "The buffer array doesn't follow the resource set layout!");

    VulkanGpuResSetUpdate(set, imageCount, images, bufferCount, buffers);
}

void* mfResourceSetLayoutGetBackend(MFResourceSetLayout* layout) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");
    MF_PANIC_IF(!layout->init, mfGetLogger(), "The resource set layout isn't initialised!");
    
    return layout->layout;
}

void** mfResourceSetGetBackend(MFResourceSet* set) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(!set->init, mfGetLogger(), "The resource set isn't initialised!");

    return (void**)set->sets;
}

size_t mfResourceSetLayoutGetSizeInBytes(void) {
    return sizeof(MFResourceSetLayout);
}
size_t mfResourceSetGetSizeInBytes(void) {
    return sizeof(MFResourceSet);
}

#ifdef __cplusplus
}
#endif
