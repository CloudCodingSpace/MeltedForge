#pragma once

#include <stddef.h>
#include <stdint.h>
#include <slog/slog.h>

typedef struct MFContext_s MFContext;

void mfInit(void);
void mfShutdown(void);

uint64_t mfGetNextID(void);

void mfSetCurrentContext(MFContext* ctx);
MFContext* mfGetCurrentContext(void);

size_t mfGetContextSizeInBytes(void);
SLogger* mfGetLogger(void);