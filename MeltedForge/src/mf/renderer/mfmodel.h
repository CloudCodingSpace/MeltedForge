#pragma once

#include "mfmesh.h"
#include "core/mfutils.h"
#include "core/mfcore.h"

#include <tinyobj/tinyobjloader_c.h>

typedef void (*MFModelVertexBuilder)(void* dst, const tinyobj_attrib_t* attrib, const tinyobj_vertex_index_t* idx);

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