#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"
#include "core/mfmaths.h"

#include "mfrender_target.h"

#include "window/mfwindow.h"

#include "mfutil_types.h"

typedef struct MFRenderer_s MFRenderer;

void mfRendererInit(MFRenderer* renderer, const char* appName, b8 enableDepth, b8 vsync, b8 enableUI, MFWindow* window);
void mfRendererShutdown(MFRenderer* renderer);

void mfRendererBeginframe(MFRenderer* renderer, MFWindow* window);
void mfRendererEndframe(MFRenderer* renderer, MFWindow* window);

void mfRendererWaitForGPU(MFRenderer* renderer);

void mfRendererSetRenderTarget(MFRenderer* renderer, MFRenderTarget* rt);

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color);
void mfRendererDrawVertices(MFRenderer* renderer, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance);
void mfRendererDrawVerticesIndexed(MFRenderer* renderer, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance);

MFViewport mfRendererGetViewport(MFRenderer* renderer);
MFRect2D mfRendererGetScissor(MFRenderer* renderer);

void* mfRendererGetBackend(MFRenderer* renderer);
void* mfRendererGetRenderPass(MFRenderer* renderer);

u8 mfRendererGetCurrentFrameIdx(MFRenderer* renderer);
f64 mfRendererGetDeltaTime(MFRenderer* renderer);
f64 mfRendererGetFrameTime(MFRenderer* renderer);

size_t mRendererGetSizeInBytes(void);
u8 mfRendererGetFramesInFlight(void);