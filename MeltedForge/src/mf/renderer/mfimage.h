#pragma once

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"

typedef struct MFGpuImage_s MFGpuImage;

struct VulkanImage_s;

typedef struct MFGpuImageConfig_s {
    u32 width;
    u32 height;
    u32 binding;
    u8* pixels;
} MFGpuImageConfig;

void mfGpuImageCreate(MFGpuImage* image, MFRenderer* renderer, MFGpuImageConfig config);
void mfGpuImageDestroy(MFGpuImage* image);

void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels);
void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height);

const MFGpuImageConfig* mfGetGpuImageConfig(MFGpuImage* image);
size_t mfGetGpuImageSizeInBytes();

void mfGpuImageSetBinding(MFGpuImage* image, u32 binding);

MFResourceDesc mfGetGpuImageDescription(MFGpuImage* image);
struct VulkanImage_s mfGetGpuImageBackend(MFGpuImage* image);