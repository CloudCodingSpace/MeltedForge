#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "window/mfwindow.h"

typedef struct MFRenderer_s MFRenderer;

void mfRendererInit(MFRenderer* renderer, const char* appName, MFWindow* window);
void mfRendererShutdown(MFRenderer* renderer);

void mfRendererBeginframe(MFRenderer* renderer);
void mfRendererEndframe(MFRenderer* renderer);

size_t mfGetRendererSizeInBytes();