#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "window/mfwindow.h"
#include "core/mfarray.h"

#include "ctx.h"
#include "common.h"
#include "fb.h"
#include "renderpass.h"

struct VulkanRenderTarget_s;

typedef struct VulkanBackendConfig_s {
    bool enableUI;
    bool enableDepth;
    bool vsync;
    bool headless;
    const char* appName;
    MFVec2 headlessExtent;
    GLFWwindow* window;
    VkSampleCountFlagBits msaaSamples;
} VulkanBackendConfig;

typedef struct VulkanBackend_s {
    VulkanBackendConfig config;
    VulkanBackendCtx ctx;
    u32 swapchainImageIndex, frameIndex;
    VkClearValue clearColor;

    VkFormat depthFormat;
    VulkanImage depthImage;

    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
    VkCommandBuffer computeCmdBuffers[FRAMES_IN_FLIGHT];

    VulkanImage* msaaImages;
    VulkanRenderPass pass;
    u32 frameBufferCount;
    VulkanFramebuffer* frameBuffers;

    VkSemaphore imageAvailableSemas[FRAMES_IN_FLIGHT];
    VkSemaphore* renderFinishedSemas;
    VkFence inFlightFences[FRAMES_IN_FLIGHT];

    const char* pipelineCacheFilePath;
    VkPipelineCache pipelineCache;
    struct VulkanRenderTarget_s* renderTarget;
    MFArray waitStages;
    MFArray waitSemas;
    MFArray descSetBindingPool;

    void* callbackState;
    void (*resizeCallback)(void* state);
} VulkanBackend;

void VulkanBackendInit(VulkanBackend* backend, VulkanBackendConfig* config);
void VulkanBackendShutdown(VulkanBackend* backend);

bool VulkanBackendBeginframe(VulkanBackend* backend);
void VulkanBackendEndframe(VulkanBackend* backend);
void VulkanBackendWaitForFrame(VulkanBackend* backend);

void VulkanBackendDrawVertices(VulkanBackend* backend, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance);
void VulkanBackendDrawVerticesIndexed(VulkanBackend* backend, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance);

void VulkanBackendSetCurrentImagePixels(VulkanBackend* backend, u8* pixels);
u8* VulkanBackendGetCurrentImagePixels(VulkanBackend* backend, u32* width, u32* height);

#ifdef __cplusplus
}
#endif