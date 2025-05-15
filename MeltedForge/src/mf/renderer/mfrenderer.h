#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"
#include "core/mfmaths.h"

#include "window/mfwindow.h"

#include "mfutil_types.h"

typedef struct MFRenderer_s MFRenderer;

void mfRendererInit(MFRenderer* renderer, const char* appName, MFWindow* window);
void mfRendererShutdown(MFRenderer* renderer);

void mfRendererBeginframe(MFRenderer* renderer, MFWindow* window);
void mfRendererEndframe(MFRenderer* renderer, MFWindow* window);

void mfRendererWait(MFRenderer* renderer);

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color);
void mfRendererDrawVertices(MFRenderer* renderer, u32 vertexCount, u32 instances, u32 firstVertex, u32 firstInstance);

MFViewport mfRendererGetViewport(const MFWindowConfig* config);
MFRect2D mfRendererGetScissor(const MFWindowConfig* config);

size_t mfGetRendererSizeInBytes();
void* mfRendererGetBackend(MFRenderer* renderer);