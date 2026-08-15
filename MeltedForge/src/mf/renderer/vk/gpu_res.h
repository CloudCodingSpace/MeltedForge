#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfarray.h"
#include "renderer/mfrenderer.h"
#include "renderer/mfgpubuffer.h"
#include "renderer/mfgpuimage.h"

#include "common.h"
#include "ctx.h"
#include "backend.h"
#include <vulkan/vulkan.h>

typedef struct VulkanGpuResDescriptorPool_s {
    VkDescriptorPoolSize sizes[11];
    VkDescriptorPool pool;
    u64 allocatedSets;
    bool isFull;
} VulkanGpuResDescriptorPool;

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    MFRenderer* renderer;
    MFArray bindings;
    bool init;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
    u64 poolIdx;
    bool init;
};

u64 VulkanGpuResCreatePool(VulkanBackendCtx* ctx, u32 poolSizeCount, VkDescriptorPoolSize* sizes, u64 maxSets);

void VulkanGpuResSetUpdate(MFResourceSet* set, u32 imageCount, MFGpuImage** images, u32 bufferCount, MFGpuBuffer** buffers);

void VulkanGpuResGetPoolSizesFromBindings(u64* poolSizes, u64 bindingCount, MFResourceSetBindings* bindings);

#ifdef __cplusplus
}
#endif
