#include "renderer/mfutil_types.h"
#ifdef __cplusplus
extern "C" {
#endif

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
#include "vk/gpu_res.h"

MFResourceSetLayout* mfResourceSetLayoutCreate(u64 bindingLen, MFResourceSetBindings* bindings, u64 maxSets, MFRenderer* renderer) {
    MF_PANIC_IF(maxSets == 0, mfGetLogger(), "The provided maxSet count shouldn't be 0!");
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

    // Descriptor pool
    {
        u32 count = 0;
        VkDescriptorPoolSize sizes[4] = {0};

        // TODO: Support more shader res type!
        u64 indices[MF_RES_DESCRIPTION_TYPE_COUNT];
        MF_SETMEM(indices, 0, sizeof(indices));
        VulkanGpuResGetPoolSizesFromBindings(indices, bindingLen, bindings);

        if(indices[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, indices[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] * FRAMES_IN_FLIGHT * ((u32)maxSets) };
        }
        if(indices[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, indices[MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER] * FRAMES_IN_FLIGHT * ((u32)maxSets) };
        }
        if(indices[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indices[MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER] * FRAMES_IN_FLIGHT * ((u32)maxSets) };
        }
        if(indices[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, indices[MF_RES_DESCRIPTION_TYPE_STORAGE_IMAGE] * FRAMES_IN_FLIGHT * ((u32)maxSets) };
        }

        layout->pool = VulkanGpuResCreatePool(ctx, count, sizes, maxSets * FRAMES_IN_FLIGHT);
    }

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

    VulkanGpuResDestroyPool(ctx, layout->pool);
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

    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = layout->pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout->layout
    };

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
        VK_CHECK(vkAllocateDescriptorSets(ctx->device, &info, &set->sets[i]));
    
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

    VK_CHECK(vkFreeDescriptorSets(ctx->device, set->layout->pool, FRAMES_IN_FLIGHT, set->sets));

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

void mfResourceSetUpdate(MFResourceSet* set, MFArray* images, MFArray* buffers) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(!set->init, mfGetLogger(), "The resource set isn't initialised!");

    u64 imgCount = 0, buffCount = 0;
    if(images != mfnull) {
        imgCount = images->len;
    }
    if(buffers != mfnull) {
        buffCount = buffers->len;
    }

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

    MF_PANIC_IF(imgCount != bindingImgCount, mfGetLogger(), "The image array doesn't follow the resource set layout!");
    MF_PANIC_IF(buffCount != bindingBuffCount, mfGetLogger(), "The buffer array doesn't follow the resource set layout!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    u64 count = imgCount + buffCount;
    VkWriteDescriptorSet* writes = MF_ALLOCMEM(VkWriteDescriptorSet, sizeof(VkWriteDescriptorSet) * count);
    VkDescriptorImageInfo* imgInfos = MF_ALLOCMEM(VkDescriptorImageInfo, sizeof(VkDescriptorImageInfo) * imgCount);
    VkDescriptorBufferInfo* buffInfos = MF_ALLOCMEM(VkDescriptorBufferInfo, sizeof(VkDescriptorBufferInfo) * buffCount);
    u32* imgBindings = MF_ALLOCMEM(u32, sizeof(u32) * imgCount);
    u32* buffBindings = MF_ALLOCMEM(u32, sizeof(u32) * buffCount);

    // Getting bindings
    {
        u64 imgIdx = 0, buffIdx = 0;
        for(u64 i = 0; i < count; i++) {
            MFResourceSetBindings* binding = &mfArrayGetElement(set->layout->bindings, MFResourceSetBindings, i);
            if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER)
                imgBindings[imgIdx++] = binding->binding;
            else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_STORAGE_IMAGE)
                imgBindings[imgIdx++] = binding->binding;
            else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER)
                buffBindings[buffIdx++] = binding->binding;
            else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER)
                buffBindings[buffIdx++] = binding->binding;
        }
    }
    
    for (u32 frame = 0; frame < FRAMES_IN_FLIGHT; frame++) {
        u32 writeIdx = 0;

        // Images
        for (u64 i = 0; i < imgCount; i++) {
            MFGpuImage* image = mfArrayGetElement(*images, MFGpuImage*, i);
            u32 idx = mfGpuImageGetConfig(image)->frameSynced ? frame : 0;
            VulkanImage* imageBackends = (VulkanImage*)mfGpuImageGetBackend(image);
            VulkanImage* imageBackend = &imageBackends[idx];

            imgInfos[i] = (VkDescriptorImageInfo){
                .imageLayout = imageBackend->info.storageImage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = imageBackend->view,
                .sampler = imageBackend->sampler
            };

            writes[writeIdx] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set->sets[frame],
                .dstBinding = imgBindings[i],
                .descriptorType = (VkDescriptorType)(u32)mfGpuImageGetDescription(image).descriptorType,
                .descriptorCount = 1,
                .pImageInfo = &imgInfos[i]
            };

            writeIdx++;
        }

        // Buffers
        for (u64 i = 0; i < buffCount; i++) {
            MFGpuBuffer* buffer = mfArrayGetElement(*buffers, MFGpuBuffer*, i);
            VulkanBuffer* bufferBackends = (VulkanBuffer*)mfGpuBufferGetBackend(buffer);
            MF_PANIC_IF(bufferBackends->info.type != VULKAN_BUFFER_TYPE_UBO && bufferBackends->info.type != VULKAN_BUFFER_TYPE_SSBO, mfGetLogger(), 
                                        "The given buffer for resource set isn't an uniform/shader storage buffer!");
            u32 idx = mfGpuBufferGetConfig(buffer)->frameSynced ? frame : 0;
            VulkanBuffer* bufferBackend = &bufferBackends[idx];

            buffInfos[i] = (VkDescriptorBufferInfo){
                .buffer = bufferBackend->handle,
                .offset = 0,
                .range = bufferBackend->info.size
            };

            writes[writeIdx] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set->sets[frame],
                .dstBinding = buffBindings[i],
                .descriptorType = (VkDescriptorType)(u32)mfGpuBufferGetDescription(buffer).descriptorType,
                .descriptorCount = 1,
                .pBufferInfo = &buffInfos[i]
            };

            writeIdx++;
        }

        if(count > 0)
            vkUpdateDescriptorSets(ctx->device, writeIdx, writes, 0, NULL);
    }

    MF_FREEMEM(writes);
    MF_FREEMEM(buffInfos);
    MF_FREEMEM(imgInfos);
    MF_FREEMEM(buffBindings);
    MF_FREEMEM(imgBindings);
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
