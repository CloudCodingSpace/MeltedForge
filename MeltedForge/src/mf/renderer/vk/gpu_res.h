#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfarray.h"
#include "renderer/mfrenderer.h"

#include "common.h"
#include "ctx.h"
#include "backend.h"
#include <vulkan/vulkan.h>

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    MFRenderer* renderer;
    MFArray bindings;
    bool init;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
    bool init;
};

VkDescriptorPool VulkanGpuResCreatePool(VulkanBackendCtx* ctx, u32 poolSizeCount, VkDescriptorPoolSize* sizes, u64 maxSets);
void VulkanGpuResDestroyPool(VulkanBackendCtx* ctx, VkDescriptorPool pool);

void VulkanGpuResGetPoolSizesFromBindings(u64* poolSizes, u64 bindingCount, MFResourceSetBindings* bindings);

#ifdef __cplusplus
}
#endif
