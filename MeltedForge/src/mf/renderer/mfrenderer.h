#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"
#include "core/mfmaths.h"

#include "window/mfwindow.h"

typedef struct MFRenderer_s MFRenderer;

void mfRendererInit(MFRenderer* renderer, const char* appName, MFWindow* window);
void mfRendererShutdown(MFRenderer* renderer);

void mfRendererBeginframe(MFRenderer* renderer, MFWindow* window);
void mfRendererEndframe(MFRenderer* renderer, MFWindow* window);

void mfRendererSetClearColor(MFRenderer* renderer, MFVec3 color);

size_t mfGetRendererSizeInBytes();