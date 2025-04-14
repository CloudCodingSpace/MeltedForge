#pragma once

#include "common.h"

#include "window/mfwindow.h"

typedef struct MFVkBackendQueueData_s {
    i32 gQueueIdx, tQueueIdx, pQueueIdx, cQueueIdx;
    VkQueue gQueue, tQueue, pQueue, cQueue;
} MFVkBackendQueueData;

typedef struct MFVkBackendCtx_s {
    VkAllocationCallbacks* callbacks;
    VkInstance instance;
    VkSurfaceKHR surface;

    MFVkBackendQueueData qData;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
} MFVkBackendCtx;

void mfVkBckndCtxInit(MFVkBackendCtx* ctx, const char* appName, MFWindow* window);
void mfVkBckndCtxDestroy(MFVkBackendCtx* ctx);