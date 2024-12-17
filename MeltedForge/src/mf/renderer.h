#pragma once

#include "backend/vulkan/vkbcknd.h"
#include "backend/backend_types.h"

typedef struct {
  MFVkBckndState vkState;
  MFBckndTypes type;
} MFRenderer;

void mfRendererInit(MFRenderer* renderer, MFBckndTypes type);
void mfRendererDeinit(MFRenderer* renderer);
