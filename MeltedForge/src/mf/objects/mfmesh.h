#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "renderer/mfrenderer.h"
#include "renderer/mfgpubuffer.h"
#include "renderer/mfgpu_res.h"

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
    const char* normal_texpath;
    const char* displacement_texpath;
    const char* shininess_texpath;
    const char* lightmap_texpath;
    const char* emission_texpath;
    const char* metalness_texpath;
    const char* alpha_texpath;
    MFResourceSet* set; // TODO: Baked resource set manually by client for now. So change this later by using a resource set helper system
} MFMeshMaterial;

typedef struct MFMesh_s {
    MFGpuBuffer* vertBuffer;
    MFGpuBuffer* indBuffer;
    MFRenderer* renderer;

    u64 vertSize;
    u32 vertCount;

    MFMat4 transform;
    MFMeshMaterial mat;
    bool init;
} MFMesh;

// @brief Creates a mesh
// @param mesh A valid pointer to a MFMesh handle
// @param renderer A valid pointer to a MFRenderer handle
// @param vertSize Size of the entire vertex buffer data in bytes
// @param vertices A valid pointer to the vertex data
// @param indicesCount Count of indices in the mesh
// @param indices A valid pointer to u32 for the index data 
void mfMeshCreate(MFMesh* mesh, MFRenderer* renderer, u64 vertSize, void* vertices, u32 indicesCount, u32* indices);

// @param Destroys a mesh
// @param mesh A valid pointer to a MFMesh handle
void mfMeshDestroy(MFMesh* mesh);

// @brief Renders a mesh
// @param mesh A valid pointer to a MFMesh handle
void mfMeshRender(MFMesh* mesh);

#ifdef __cplusplus
}
#endif