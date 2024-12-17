#include "renderer.h"

#include "mf/backend/vulkan/vkbcknd.h"

void mfRendererInit(MFRenderer* renderer, MFBckndTypes type) {
  renderer->type = type;

  switch(type) {
    case MFBCKND_TYPE_VULKAN:
      mfVkBckndInit(&renderer->vkState);
      break;
  } 
}

void mfRendererDeinit(MFRenderer* renderer) {
  
}
