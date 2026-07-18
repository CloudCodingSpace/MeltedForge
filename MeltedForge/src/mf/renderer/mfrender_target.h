#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfmaths.h"

#include "mfgpu_res.h"

#include <cimgui.h>

struct MFRenderer_s;
struct MFPipeline_s;

typedef struct MFRenderTarget_s MFRenderTarget;

MFRenderTarget* mfRenderTargetCreate(struct MFRenderer_s* renderer, bool hasDepth);
void mfRenderTargetDestroy(MFRenderTarget* renderTarget);

void mfRenderTargetResize(MFRenderTarget* renderTarget, MFVec2 extent);

void mfRenderTargetSetClearColor(MFRenderTarget* renderTarget, MFVec3 color);
MFVec3 mfRenderTargetGetClearColor(MFRenderTarget* renderTarget);

void mfRenderTargetBegin(MFRenderTarget* renderTarget);
void mfRenderTargetEnd(MFRenderTarget* renderTarget, bool waitOnCpu);

void mfRenderTargetSetResizeCallback(MFRenderTarget* renderTarget, void (*callback)(void* userData), void* userData);
void* mfRenderTargetGetPass(MFRenderTarget* renderTarget);

u8* mfRenderTargetGetCurrentImagePixels(MFRenderTarget* renderTarget, u32* width, u32* height);
u32 mfRenderTargetGetWidth(MFRenderTarget* renderTarget);
u32 mfRenderTargetGetHeight(MFRenderTarget* renderTarget);

MFResourceSetLayout* mfRenderTargetGetResourceSetLayout(MFRenderTarget* renderTarget);
void mfRenderTargetBindAttachmentResourceSets(MFRenderTarget* renderTarget, u64 setIndex, struct MFPipeline_s* pipeline);
ImTextureID mfRenderTargetGetColorAttachmentImTexID(MFRenderTarget* renderTarget);
size_t mfRenderTargetGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif