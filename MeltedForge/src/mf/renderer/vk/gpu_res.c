#ifdef __cplusplus
extern "C" {
#endif

#include "gpu_res.h"
#include "renderer/mfutil_types.h"

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
