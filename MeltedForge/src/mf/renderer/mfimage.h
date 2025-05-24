#pragma once

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"

typedef struct MFGpuImage_s MFGpuImage;

struct VulkanImage_s;

typedef struct MFGpuImageConfig_s {
    u32 width;
    u32 height;
    u8* pixels;
} MFGpuImageConfig;

void mfGpuImageCreate(MFGpuImage* image, MFRenderer* renderer, MFGpuImageConfig config);
void mfGpuImageDestroy(MFGpuImage* image);

void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels);
void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height);

const MFGpuImageConfig* mfGetGpuImageConfig(MFGpuImage* image);
size_t mfGetGpuImageSizeInBytes();

MFResourceDesc mfGetGpuImageDescription(u32 binding);
struct VulkanImage_s mfGetGpuImageBackend(MFGpuImage* image);