#ifdef __cplusplus
extern "C" {
#endif

#include "gpu_res.h"
#include "buffer.h"
#include "image.h"

#include "renderer/mfutil_types.h"
#include "renderer/mfgpubuffer.h"
#include "renderer/mfgpuimage.h"

u64 VulkanGpuResCreatePool(VulkanBackendCtx* ctx, u32 poolSizeCount, VkDescriptorPoolSize* sizes, u64 maxSets) {
    u64 idx = UINT64_MAX;

    for(u64 i = 0; i < ctx->descriptorPools.len; i++) {
        VulkanGpuResDescriptorPool* descPool = &mfArrayGetElement(ctx->descriptorPools, VulkanGpuResDescriptorPool, i);
        if(descPool->isFull)
            continue;

        bool supported = descPool->allocatedSets + maxSets > VULKAN_GPU_RES_MAX_DESCRIPTORS;
        for(u64 j = 0; j < poolSizeCount; j++) {
            VkDescriptorPoolSize sj = sizes[j];
            supported = supported && ((sj.descriptorCount + descPool->sizes[sj.type].descriptorCount) > VULKAN_GPU_RES_MAX_DESCRIPTORS);
        }

        if(supported) {
            idx = i;
            descPool->allocatedSets += maxSets;
            
            for(u64 j = 0; j < poolSizeCount; j++) {
                descPool->sizes[sizes[j].type].descriptorCount += sizes[j].descriptorCount;
            }
        }
    }

    if(idx == UINT64_MAX) {
        VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VULKAN_GPU_RES_MAX_DESCRIPTORS },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VULKAN_GPU_RES_MAX_DESCRIPTORS } 
		};

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = MF_ARRAYLEN(poolSizes),
            .pPoolSizes = poolSizes,
            .maxSets = VULKAN_GPU_RES_MAX_DESCRIPTORS,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        };
    
        VkDescriptorPool p = mfnull;
        VK_CHECK(vkCreateDescriptorPool(ctx->device, &info, ctx->allocator, &p));

        VulkanGpuResDescriptorPool pool = {
            .pool = p
        };
        for(u32 i = 0; i < 11; i++)
            pool.sizes[i].type = (VkDescriptorType)i;
        for(u32 i = 0; i < poolSizeCount; i++) {
            pool.sizes[sizes[i].type] = sizes[i];
        }
        idx = ctx->descriptorPools.len;
        mfArrayAddElement(&ctx->descriptorPools, VulkanGpuResDescriptorPool, pool);
    }

    return idx;
}

void VulkanGpuResSetUpdate(MFResourceSet* set, u32 imageCount, MFGpuImage** images, u32 bufferCount, MFGpuBuffer** buffers) {
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    u64 count = imageCount + bufferCount;
    VkWriteDescriptorSet* writes = MF_ALLOCMEM(VkWriteDescriptorSet, sizeof(VkWriteDescriptorSet) * count);
    VkDescriptorImageInfo* imgInfos = MF_ALLOCMEM(VkDescriptorImageInfo, sizeof(VkDescriptorImageInfo) * imageCount);
    VkDescriptorBufferInfo* buffInfos = MF_ALLOCMEM(VkDescriptorBufferInfo, sizeof(VkDescriptorBufferInfo) * bufferCount);
    u32* imgBindings = MF_ALLOCMEM(u32, sizeof(u32) * imageCount);
    u32* buffBindings = MF_ALLOCMEM(u32, sizeof(u32) * bufferCount);

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
        for (u64 i = 0; i < imageCount; i++) {
            MFGpuImage* image = images[i];
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
        for (u64 i = 0; i < bufferCount; i++) {
            MFGpuBuffer* buffer = buffers[i];
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
