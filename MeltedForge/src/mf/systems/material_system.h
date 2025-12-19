#pragma once

#include "core/mfutils.h"
#include "renderer/mfmodel.h"

typedef enum MFModelMatTextures_e {
    MF_MODEL_MAT_TEXTURE_AMBIENT = 0,
    MF_MODEL_MAT_TEXTURE_DIFFUSE,
    MF_MODEL_MAT_TEXTURE_SPECULAR,
    MF_MODEL_MAT_TEXTURE_BUMP,
    MF_MODEL_MAT_TEXTURE_DISPLACEMENT,
    MF_MODEL_MAT_TEXTURE_LIGHTMAP,
    MF_MODEL_MAT_TEXTURE_METALNESS,
    MF_MODEL_MAT_TEXTURE_SHININESS,
    MF_MODEL_MAT_TEXTURE_EMISSIVE,
    MF_MODEL_MAT_TEXTURE_ALPHA,
    MF_MODEL_MAT_TEXTURE_MAX
} MFModelMatTextures;

MFArray mfMaterialSystemGetModelMatImages(MFModel* model, const char* basePath, void* renderer);
void mfMaterialSystemDeleteModelMatImages(MFArray* array);