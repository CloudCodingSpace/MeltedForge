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

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    MFRenderer* renderer;
    MFArray resourceDescriptions;
    u64 imageCount, bufferCount;
    bool init;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
    bool init;
};

MFResourceSetLayout* mfResourceSetLayoutCreate(u64 reourceDescriptionLen, MFResourceDescription* resourceDescriptions, u64 maxSets, MFRenderer* renderer) {
    MF_PANIC_IF(maxSets == 0, mfGetLogger(), "The provided maxSet count shouldn't be 0!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The provided renderer handle shouldn't be null!");
    MF_PANIC_IF(reourceDescriptionLen == 0, mfGetLogger(), "The provided resource description count shouldn't be 0!");
    MF_PANIC_IF(resourceDescriptions == mfnull, mfGetLogger(), "The provided resource descriptions shouldn't be null!");

    MFResourceSetLayout* layout = MF_ALLOCMEM(MFResourceSetLayout, sizeof(MFResourceSetLayout));

    layout->renderer = renderer;
    layout->resourceDescriptions = mfArrayCreate(mfGetLogger(), reourceDescriptionLen, sizeof(MFResourceDescription));
    layout->resourceDescriptions.len = reourceDescriptionLen;

    for(u64 i = 0; i < reourceDescriptionLen; i++) {
        mfArraySetElement(layout->resourceDescriptions, MFResourceDescription, i, resourceDescriptions[i]);
    }

    // TODO: Support more shader res type!
    u64 imageCount = 0, bufferCount = 0;
    for(u64 i = 0; i < reourceDescriptionLen; i++) {
        if(resourceDescriptions[i].descriptorType == MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER) {
            imageCount++;
        }
        if(resourceDescriptions[i].descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER) {
            bufferCount++;
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
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, ((u32)imageCount) * FRAMES_IN_FLIGHT * ((u32)maxSets) };
        }
        if(bufferCount != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ((u32)bufferCount) * FRAMES_IN_FLIGHT * ((u32)maxSets) };
        }

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = maxSets * FRAMES_IN_FLIGHT,
            .poolSizeCount = count,
            .pPoolSizes = sizes,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        };

        VK_CHECK(vkCreateDescriptorPool(ctx->device, &info, ctx->allocator, &layout->pool));
    }

    // Descriptor layout
    {
        VkDescriptorSetLayoutBinding* layBindings = MF_ALLOCMEM(VkDescriptorSetLayoutBinding, sizeof(VkDescriptorSetLayoutBinding) * reourceDescriptionLen);
        u32 uniqueBindingCount = 0;

        for(u32 i = 0; i < reourceDescriptionLen; i++) {
            bool bindingExists = false;
            for(u32 j = 0; j < uniqueBindingCount; j++) {
                if(layBindings[j].binding == resourceDescriptions[i].binding) {
                    bindingExists = true;
                    break;
                }
            }

            if(!bindingExists) {
                layBindings[uniqueBindingCount].binding = resourceDescriptions[i].binding;
                layBindings[uniqueBindingCount].descriptorType = (VkDescriptorType)((int)resourceDescriptions[i].descriptorType);
                layBindings[uniqueBindingCount].descriptorCount = resourceDescriptions[i].descriptorCount;
                layBindings[uniqueBindingCount].stageFlags = (VkShaderStageFlags)((int)resourceDescriptions[i].stageFlags);
                uniqueBindingCount++;
            }
        }

        MF_DO_IF(uniqueBindingCount != reourceDescriptionLen, {
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

    vkDestroyDescriptorPool(ctx->device, layout->pool, ctx->allocator);
    vkDestroyDescriptorSetLayout(ctx->device, layout->layout, ctx->allocator);

    mfArrayDestroy(&layout->resourceDescriptions, mfGetLogger());

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
        MF_PANIC_IF(!sets[i]->init, mfGetLogger(), "The resource set isn't initialised!");
    }

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(sets[0]->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VkCommandBuffer buff = backend->commandBuffers[backend->frameIndex];
    if(backend->renderTarget != mfnull) {
        buff = backend->renderTarget->commandBuffers[backend->frameIndex];
    }

    VulkanPipeline* pipelineBackend = (VulkanPipeline*)mfPipelineGetBackend(pipeline);

    // TODO: Later fix this by figuring out a way to not allocate this list in the heap and freeing it per bind
    VkDescriptorSet* handles = MF_ALLOCMEM(VkDescriptorSet, sizeof(VkDescriptorSet) * setCount);
    for(u64 i = 0; i < setCount; i++) {
        handles[i] = sets[i]->sets[backend->frameIndex];
    }

    vkCmdBindDescriptorSets(buff, pipelineBackend->bindPoint, pipelineBackend->layout, 
                                    firstSetIndex, setCount, handles, 
                                    0, mfnull);

    MF_FREEMEM(handles);
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

    MF_PANIC_IF(imgCount != set->layout->imageCount, mfGetLogger(), "The image array doesn't follow the resource set layout!");
    MF_PANIC_IF(buffCount != set->layout->bufferCount, mfGetLogger(), "The buffer array doesn't follow the resource set layout!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    u64 count = set->layout->imageCount + set->layout->bufferCount;
    VkWriteDescriptorSet* writes = MF_ALLOCMEM(VkWriteDescriptorSet, sizeof(VkWriteDescriptorSet) * count);
    VkDescriptorImageInfo* imgInfos = MF_ALLOCMEM(VkDescriptorImageInfo, sizeof(VkDescriptorImageInfo) * set->layout->imageCount);
    VkDescriptorBufferInfo* buffInfos = MF_ALLOCMEM(VkDescriptorBufferInfo, sizeof(VkDescriptorBufferInfo) * set->layout->bufferCount);

    for(u64 i = 0; i < set->layout->imageCount; i++) {
        VulkanImage* image = (VulkanImage*)mfGpuImageGetBackend(mfArrayGetElement(*images, MFGpuImage*, i));
        imgInfos[i] = (VkDescriptorImageInfo){
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = image->view,
            .sampler = image->sampler
        };
    }
    
    for (u32 frame = 0; frame < FRAMES_IN_FLIGHT; frame++) {
        u32 writeIdx = 0;

        // Images
        for (u64 i = 0; i < set->layout->imageCount; i++) {
            writes[writeIdx] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set->sets[frame],
                .dstBinding = mfGpuImageGetDescription(mfArrayGetElement(*images, MFGpuImage*, i)).binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &imgInfos[i]
            };
            writeIdx++;
        }

        // Buffers
        for (u64 i = 0; i < set->layout->bufferCount; i++) {
            VulkanBuffer* buffer = (VulkanBuffer*)mfGpuBufferGetBackend(mfArrayGetElement(*buffers, MFGpuBuffer*, i));
            MF_PANIC_IF(buffer->type != VULKAN_BUFFER_TYPE_UBO, mfGetLogger(), 
                                        "The given buffer for resource set isn't of an uniform buffer!");

            buffInfos[i] = (VkDescriptorBufferInfo){
                .buffer = buffer[frame].handle,
                .offset = 0,
                .range = buffer[frame].size
            };

            writes[writeIdx] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set->sets[frame],
                .dstBinding = mfGpuBufferGetDescription(mfArrayGetElement(*buffers, MFGpuBuffer*, i)).binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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