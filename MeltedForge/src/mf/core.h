#pragma once

#include <slog/slog.h>

#include "renderer.h"

typedef struct {
  SLogger logger;
  MFRenderer renderer;
} MFState;

void mfInitialize();
void mfShutdown();

MFRenderer* mfGetRendererHandle();
SLogger* mfGetSLoggerHandle();
