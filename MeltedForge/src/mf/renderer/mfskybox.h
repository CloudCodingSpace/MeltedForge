#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfrenderer.h"
#include "mfutil_types.h"

typedef struct MFSkybox_s MFSkybox;

typedef struct MFSkyboxConfig_s {
    const char* hdrEnvironmentPath;
    u32 binding;
    u64 faceSize;
} MFSkyboxConfig;

MFSkybox* mfSkyboxCreate(MFSkyboxConfig config, MFRenderer* renderer);
void mfSkyboxDestroy(MFSkybox* skybox);

MFResourceDescription mfSkyboxGetDescription(MFSkybox* skybox);
void mfSkyboxSetBinding(MFSkybox* skybox, u64 binding);

void* mfSkyboxGetBackend(MFSkybox* skybox);
size_t mfSkyboxGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif