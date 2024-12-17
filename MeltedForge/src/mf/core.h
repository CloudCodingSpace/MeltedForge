#pragma once

#include <slog/slog.h>

#include "renderer.h"

typedef struct {
  SLogger logger;
  MFRenderer renderer;
} MFState;

void mfInitialize(MFBckndTypes type);
void mfShutdown();

MFRenderer* mfGetRendererHandle();
SLogger* mfGetSLoggerHandle();
