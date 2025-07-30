#pragma once

#include <stddef.h>
#include <slog/slog.h>

typedef struct MFContext_s MFContext;

void mfInit(void);
void mfShutdown(void);

void mfSetCurrentContext(MFContext* ctx);
MFContext* mfCheckCurrentContext(void);

size_t mfGetContextSizeInBytes(void);
SLogger* mfGetLogger(void);