#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "objects/mfmodel.h"

#include "renderer/mfgpuimage.h"

typedef enum MFModelMatTextures_e {
    MF_MODEL_MAT_TEXTURE_AMBIENT = 0,
    MF_MODEL_MAT_TEXTURE_DIFFUSE,
    MF_MODEL_MAT_TEXTURE_SPECULAR,
    MF_MODEL_MAT_TEXTURE_NORMAL,
    MF_MODEL_MAT_TEXTURE_DISPLACEMENT,
    MF_MODEL_MAT_TEXTURE_LIGHTMAP,
    MF_MODEL_MAT_TEXTURE_METALNESS,
    MF_MODEL_MAT_TEXTURE_SHININESS,
    MF_MODEL_MAT_TEXTURE_EMISSIVE,
    MF_MODEL_MAT_TEXTURE_ALPHA,
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