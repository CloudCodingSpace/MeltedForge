#pragma once

#include "core/mfutils.h"
#include "core/mfmaths.h"

struct MFRenderer_s;

typedef struct MFRenderTarget_s MFRenderTarget;

void mfRenderTargetCreate(MFRenderTarget* rt, struct MFRenderer_s* renderer, b8 hasDepth);
void mfRenderTargetDestroy(MFRenderTarget* rt);

void mfRenderTargetResize(MFRenderTarget* rt, MFVec2 extent);

void mfRenderTargetBegin(MFRenderTarget* rt);
void mfRenderTargetEnd(MFRenderTarget* rt);

u32 mfRenderTargetGetWidth(MFRenderTarget* rt);
u32 mfRenderTargetGetHeight(MFRenderTarget* rt);

void* mfRenderTargetGetHandle(MFRenderTarget* rt);
size_t mfGetRenderTargetSizeInBytes();