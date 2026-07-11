#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"

#include <cimgui.h>

typedef struct MFGpuImage_s MFGpuImage;

typedef struct MFGpuImageConfig_s {
    u32 width;
    u32 height;
    void* pixels;
    MFFormat imageFormat;
    MFSamplerAddressMode addressMode;
    bool generateMipmaps;
    bool isCubemap;
    bool isColorAttachment;
    bool isStorageImage;
    bool forImguiTexture;
    bool frameSynced;
} MFGpuImageConfig;

MFGpuImage* mfGpuImageCreate(MFRenderer* renderer, MFGpuImageConfig config);
void mfGpuImageDestroy(MFGpuImage* image);

ImTextureID mfGpuImageGetImGuiTextureID(MFGpuImage* image);
void mfGpuImageSetPixels(MFGpuImage* image, u8* pixels);
u8* mfGpuImageGetPixels(MFGpuImage* image, u32* width, u32* height, u32 mipLevel, u32 faceIndex);
void mfGpuImageResize(MFGpuImage* image, u32 width, u32 height, u8* pixels);

const MFGpuImageConfig* mfGpuImageGetConfig(MFGpuImage* image);
bool mfGpuImageIsValid(MFGpuImage* image);
size_t mfGpuImageGetSizeInBytes(void);

MFResourceDescription mfGpuImageGetDescription(MFGpuImage* image);
void* mfGpuImageGetBackend(MFGpuImage* image);

MFGpuImage* mfCreateErrorGpuImage(MFRenderer* renderer);

#ifdef __cplusplus
}
#endif