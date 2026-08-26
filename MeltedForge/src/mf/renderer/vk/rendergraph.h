#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../mfrenderer.h"
#include "../mfrendergraph.h"
#include "../mfgpu_res.h"

#include "image.h"
#include "gpu_res.h"
#include "fb.h"
#include "common.h"

// TODO: For now only definition in backend, but in future add a VulkanRenderGraph struct and rendergraph.c file in the vk backend

struct MFRenderGraph_s {
    MFRenderGraphConfig config;
    bool init;
    MFRenderer* renderer;
    VkClearValue* clearValues;
    VulkanImage* attachments;
    VkFramebuffer* fbs;
    VkRenderPass* passes;

    u64 attachmentSetPool;
    MFResourceSetLayout* attachmentSetLayout;
    VkDescriptorSet* igAttachmentSets;
    VkDescriptorSet* inputSets;
    VkDescriptorSet attachmentSets[FRAMES_IN_FLIGHT];

    VkSemaphore* renderFinishedSemas;
    VkFence fences[FRAMES_IN_FLIGHT];
    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
};

#ifdef __cplusplus
}
#endif