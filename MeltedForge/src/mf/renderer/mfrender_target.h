#pragma once

#include "core/mfutils.h"

struct MFRenderer_s;

typedef struct MFRenderTarget_s MFRenderTarget;

void mfRenderTargetCreate(MFRenderTarget* rt, struct MFRenderer_s* renderer, b8 hasDepth);
void mfRenderTargetDestroy(MFRenderTarget* rt);

void mfRenderTargetBegin(MFRenderTarget* rt);
void mfRenderTargetEnd(MFRenderTarget* rt);

u32 mfRenderTargetGetWidth(MFRenderTarget* rt);
u32 mfRenderTargetGetHeight(MFRenderTarget* rt);

void* mfRenderTargetGetHandle(MFRenderTarget* rt);
size_t mfGetRenderTargetSizeInBytes();