#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"

typedef struct MFGpuImage_s MFGpuImage;

typedef struct MFGpuImageConfig_s {
    u32 width;
    u32 height;
    u32 binding;
    u8* pixels;
    MFFormat imageFormat;
    bool generateMipmaps;
} MFGpuImageConfig;

void mfGpuImageCreate(MFGpuImage* image, MFRenderer* renderer, MFGpuImageConfig config);
void mfGpuImageDestroy(MFGpuImage* image);

void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels);
void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height);

const MFGpuImageConfig* mfGpuImageGetConfig(MFGpuImage* image);
size_t mfGpuImageGetSizeInBytes(void);

void mfGpuImageSetBinding(MFGpuImage* image, u32 binding);

MFResourceDescription mfGpuImageGetDescription(MFGpuImage* image);
void* mfGpuImageGetBackend(MFGpuImage* image);

MFGpuImage* mfCreateErrorGpuImage(MFRenderer* renderer);

#ifdef __cplusplus
}
#endif