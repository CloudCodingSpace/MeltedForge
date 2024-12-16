#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkInstance instance;
} MFVkBckndState;

void mfVkBckndInit();
void mfVkBckndDeinit();

MFVkBckndState* mfGetVkBckndState();
