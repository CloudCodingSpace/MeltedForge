#pragma once

#include "common.h"

#include "window/mfwindow.h"

typedef struct MFVkBackendCtx_s {
    VkAllocationCallbacks* callbacks;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
} MFVkBackendCtx;

void mfVkBckndCtxInit(MFVkBackendCtx* ctx, const char* appName, MFWindow* window);
void mfVkBckndCtxDestroy(MFVkBackendCtx* ctx);