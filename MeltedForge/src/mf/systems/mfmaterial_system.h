#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfarray.h"
#include "objects/mfmodel.h"

#include "renderer/mfgpuimage.h"

typedef enum MFModelMatTextures_e {
    MF_MODEL_MAT_TEXTURE_DIFFUSE = 0,
    MF_MODEL_MAT_TEXTURE_NORMAL,
    MF_MODEL_MAT_TEXTURE_LIGHTMAP,
    MF_MODEL_MAT_TEXTURE_EMISSIVE,
    MF_MODEL_MAT_TEXTURE_METALLIC_ROUGHNESS,
    MF_MODEL_MAT_TEXTURE_MAX
} MFModelMatTextures;

void mfMaterialSystemInitialize(void);
void mfMaterialSystemShutdown(void);

MFArray mfMaterialSystemLoadModelMatImages(MFModel* model, const char* basePath, MFRenderer* renderer);
void mfMaterialSystemDestroyModelMatImages(MFArray* array);

MFGpuImage* mfMaterialSystemGetImageFromArray(MFModelMatTextures type, MFArray* array, MFModel* model, u64 meshIdx, MFRenderer* renderer);

#ifdef __cplusplus
}
#endif