#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfarray.h"

#include "mfrenderer.h"
#include "mfgpuimage.h"
#include "mfutil_types.h"

typedef struct MFSkybox_s MFSkybox;

typedef enum MFSkyboxType_e {
    MF_SKYBOX_TYPE_NORMAL,
    MF_SKYBOX_TYPE_PREFILTERED,
    MF_SKYBOX_TYPE_IRRADIANCE
} MFSkyboxType;

typedef struct MFSkyboxConfig_s {
    const char* environmentPath;
    u32 binding;
    u64 faceSize;
    MFRenderTarget* renderTarget;
    bool generatePbrMaps, generateMipmaps;
} MFSkyboxConfig;

MFSkybox* mfSkyboxCreate(MFSkyboxConfig config, MFRenderer* renderer);
void mfSkyboxDestroy(MFSkybox* skybox);

void mfSkyboxRender(MFSkybox* skybox, MFMat4 projection, MFMat4 view, MFMat4 model, MFSkyboxType type);

// @note The returned MFGpuImage* is read only!
MFGpuImage* mfSkyboxGetCubemapImage(MFSkybox* skybox);
MFGpuImage* mfSkyboxGetIrradianceCubemapImage(MFSkybox* skybox);
size_t mfSkyboxGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif