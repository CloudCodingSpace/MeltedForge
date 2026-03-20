#pragma once

#include "renderer/mfrenderer.h"
#include "renderer/mfgpubuffer.h"

#include "core/mfutils.h"

typedef struct MFMeshMaterial_s {
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float emission[3];
    float shininess;
    float ior;  // index of refraction
    bool opaque;

    const char* ambient_texpath;
    const char* diffuse_texpath;
    const char* specular_texpath;
    const char* bump_texpath;
    const char* displacement_texpath;
    const char* shininess_texpath;
    const char* lightmap_texpath;
    const char* emission_texpath;
    const char* metalness_texpath;
    const char* alpha_texpath;
} MFMeshMaterial;

typedef struct MFMesh_s {
    MFGpuBuffer* vertBuffer;
    MFGpuBuffer* indBuffer;
    MFRenderer* renderer;

    u64 vertSize;
    u32 vertCount;

    MFMeshMaterial mat;
    b8 init;
} MFMesh;

void mfMeshCreate(MFMesh* mesh, MFRenderer* renderer, u64 vertSize, void* vertices, u32 indCount, u32* indices);
void mfMeshDestroy(MFMesh* mesh);

void mfMeshRender(MFMesh* mesh);