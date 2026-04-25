#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfmaths.h"

#include <cimgui.h>

struct MFRenderer_s;

typedef struct MFRenderTarget_s MFRenderTarget;

MFRenderTarget* mfRenderTargetCreate(struct MFRenderer_s* renderer, bool hasDepth);
void mfRenderTargetDestroy(MFRenderTarget* renderTarget);

void mfRenderTargetResize(MFRenderTarget* renderTarget, MFVec2 extent);

void mfRenderTargetSetClearColor(MFRenderTarget* renderTarget, MFVec3 color);
MFVec3 mfRenderTargetGetClearColor(MFRenderTarget* renderTarget);

void mfRenderTargetBegin(MFRenderTarget* renderTarget);
void mfRenderTargetEnd(MFRenderTarget* renderTarget);

void mfRenderTargetSetResizeCallback(MFRenderTarget* renderTarget, void (*callback)(void* userData), void* userData);
void* mfRenderTargetGetPass(MFRenderTarget* renderTarget);

u32 mfRenderTargetGetWidth(MFRenderTarget* renderTarget);
u32 mfRenderTargetGetHeight(MFRenderTarget* renderTarget);

ImTextureID mfRenderTargetGetColorAttachmentImTexID(MFRenderTarget* renderTarget);
size_t mfRenderTargetGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif