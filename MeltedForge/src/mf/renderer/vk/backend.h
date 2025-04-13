#pragma once

#include "window/mfwindow.h"

#include <vulkan/vulkan.h>

typedef struct MFVkBackend_s {
    VkInstance instance;
    VkAllocationCallbacks* callbacks;
    VkSurfaceKHR surface;
} MFVkBackend;

void mfVkBckndInit(MFVkBackend* backend, const char* appName, MFWindow* window);
void mfVkBckndShutdown(MFVkBackend* backend);

void mfVkBckndBeginframe(MFVkBackend* backend);
void mfVkBckndEndframe(MFVkBackend* backend);