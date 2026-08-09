#ifdef __cplusplus
extern "C" {
#endif

#include "gpu_res.h"
#include "buffer.h"
#include "image.h"

#include "renderer/mfutil_types.h"
#include "renderer/mfgpubuffer.h"
#include "renderer/mfgpuimage.h"

VkDescriptorPool VulkanGpuResCreatePool(VulkanBackendCtx* ctx, u32 poolSizeCount, VkDescriptorPoolSize* sizes, u64 maxSets) {
    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = poolSizeCount,
        .pPoolSizes = sizes,
        .maxSets = maxSets,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    };

    VkDescriptorPool pool = mfnull;
    VK_CHECK(vkCreateDescriptorPool(ctx->device, &info, ctx->allocator, &pool));
    return pool;
}

void VulkanGpuResDestroyPool(VulkanBackendCtx* ctx, VkDescriptorPool pool) {
    vkDestroyDescriptorPool(ctx->device, pool, ctx->allocator);
}

void VulkanGpuResSetUpdate(MFResourceSet* set, MFArray* images, MFArray* buffers) {
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    u64 imgCount = 0, buffCount = 0;
    if(images != mfnull)
        imgCount = images->len;
    if(buffers != mfnull)
        buffCount = buffers->len;
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

void VulkanGpuResGetPoolSizesFromBindings(u64* poolSizes, u64 bindingCount, MFResourceSetBindings* bindings) {
    for(u64 i = 0; i < bindingCount; i++) {
        MFResourceSetBindings* binding = &bindings[i];
        if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER]++;
        } else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_STORAGE_IMAGE) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_STORAGE_IMAGE]++;
        } else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER]++;
        } else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER]++;
        } else if(binding->description.descriptorType == MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER_DYNAMIC) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_STORAGE_BUFFER_DYNAMIC]++;
        } else if(bindings->description.descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER_DYNAMIC]++;
        } else if(bindings->description.descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_TEXEL_BUFFER) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_UNIFORM_TEXEL_BUFFER]++;
        } else if(bindings->description.descriptorType == MF_RES_DESCRIPTION_TYPE_STORAGE_TEXEL_BUFFER) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_STORAGE_TEXEL_BUFFER]++;
        } else if(bindings->description.descriptorType == MF_RES_DESCRIPTION_TYPE_INPUT_ATTACHMENT) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_INPUT_ATTACHMENT]++;
        } else if(bindings->description.descriptorType == MF_RES_DESCRIPTION_TYPE_SAMPLED_IMAGE) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_SAMPLED_IMAGE]++;
        } else if(bindings->description.descriptorType == MF_RES_DESCRIPTION_TYPE_SAMPLER) {
            poolSizes[MF_RES_DESCRIPTION_TYPE_SAMPLER]++;
        }
    }
}

#ifdef __cplusplus
}
#endif
