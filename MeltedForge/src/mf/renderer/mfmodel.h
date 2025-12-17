#pragma once

#include "mfmesh.h"
#include "core/mfutils.h"
#include "core/mfcore.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <tinyobj/tinyobjloader_c.h>

typedef struct MFModelVertexBuilderData_s {
    MFVec3 pos;
    MFVec3 normal;
    MFVec3 tangent;
    MFVec2 texCoord;
} MFModelVertexBuilderData;

typedef void (*MFModelVertexBuilder)(void* dst, MFModelVertexBuilderData data);

typedef struct MFModelMaterial_s {
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float transmittance[3];
    float emission[3];
    float shininess;
    float ior;  // index of refraction
    bool opaque;
    int illum; // illumination model (see http://www.fileformat.info/format/material/)

    const char* ambient_texpath;
    const char* diffuse_texpath;
    const char* specular_texpath;
    const char* specular_highlight_texpath;
    const char* bump_texpath;
    const char* displacement_texpath;
    const char* alpha_texpath;
} MFModelMaterial;

typedef struct MFModel_s {
    u64 meshCount;
    MFMesh* meshes;
    MFModelMaterial mat;
} MFModel;

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder);
void mfModelDestroy(MFModel* model);