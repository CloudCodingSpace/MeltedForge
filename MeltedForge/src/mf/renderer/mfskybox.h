#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "objects/mfmesh.h"

#include "mfrenderer.h"
#include "mfgpuimage.h"
#include "mfutil_types.h"

typedef struct MFSkybox_s MFSkybox;

typedef struct MFSkyboxConfig_s {
    const char* hdrEnvironmentPath;
    u32 binding;
    u64 faceSize;
} MFSkyboxConfig;

MFSkybox* mfSkyboxCreate(MFSkyboxConfig config, MFRenderer* renderer);
void mfSkyboxDestroy(MFSkybox* skybox);

void mfSkyboxRender(MFSkybox* skybox, MFMat4 projection, MFMat4 view, MFMat4 model);

// @note The returned MFGpuImage* is read only!
MFGpuImage* mfSkyboxGetCubemapImage(MFSkybox* skybox);
size_t mfSkyboxGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif