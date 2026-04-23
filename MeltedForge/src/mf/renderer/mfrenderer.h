#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfutils.h"
#include "core/mfmaths.h"

#include "mfrender_target.h"

#include "window/mfwindow.h"

#include "mfutil_types.h"

typedef struct MFRenderer_s MFRenderer;

void mfRendererInit(MFRenderer* renderer, const char* appName, bool enableDepth, bool vsync, bool enableUI, MFWindow* window);
void mfRendererShutdown(MFRenderer* renderer);

bool mfRendererBeginframe(MFRenderer* renderer, MFWindow* window);
void mfRendererEndframe(MFRenderer* renderer, MFWindow* window);

void mfRendererWaitForGPU(MFRenderer* renderer);

void mfRendererSetResizeCallback(MFRenderer* renderer, void* state, void (*callback)(void* state));

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color);
MFVec3 mfRendererGetClearColor(MFRenderer* renderer);
void mfRendererDrawVertices(MFRenderer* renderer, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance);
void mfRendererDrawVerticesIndexed(MFRenderer* renderer, u32 indexCount, u32 instances, u32 firstIndex, u32 firstInstance);

MFViewport mfRendererGetViewport(MFRenderer* renderer);
MFRect2D mfRendererGetScissor(MFRenderer* renderer);

void* mfRendererGetBackend(MFRenderer* renderer);
void* mfRendererGetRenderPass(MFRenderer* renderer);

u8 mfRendererGetCurrentFrameIdx(MFRenderer* renderer);
f64 mfRendererGetDeltaTime(MFRenderer* renderer);

size_t mfRendererGetSizeInBytes(void);
u8 mfRendererGetFramesInFlightCount(void);

#ifdef __cplusplus
}
#endif