#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MF_PLATFORM_WINDOWS) && !defined(MF_PLATFORM_LINUX) && !defined(MF_PLATFORM_MAC)
    #error MeltedForge is only supported on Windows, Linux & Mac, and not any other platforms!
#endif

#include <stddef.h>
#include <stdint.h>
#include <slog/slog.h>

void mfInitialize(void);
void mfShutdown(void);

uint64_t mfGetNextID(void);
uint8_t* mfGetErrorImagePixels(void);
uint32_t mfGetErrorImageWidth(void);
uint32_t mfGetErrorImageHeight(void);

SLogger* mfGetLogger(void);

#ifdef __cplusplus
}
#endif
