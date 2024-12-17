#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkInstance instance;

#ifdef _DEBUG
  VkDebugUtilsMessengerEXT dbgMssngr;
#endif
} MFVkBckndState;

void mfVkBckndInit(MFVkBckndState* state);
void mfVkBckndDeinit(MFVkBckndState* state); 
