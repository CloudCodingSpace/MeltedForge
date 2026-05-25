#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfarray.h"
#include "renderer/mfrenderer.h"

#include "common.h"
#include <vulkan/vulkan.h>

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    MFRenderer* renderer;
    MFArray bindings;
    u64 imageCount, bufferCount;
    bool init;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
    bool init;
};

#ifdef __cplusplus
}
#endif