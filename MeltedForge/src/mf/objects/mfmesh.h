#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "renderer/mfrenderer.h"
#include "renderer/mfgpubuffer.h"
#include "renderer/mfgpu_res.h"

#include "core/mfutils.h"

typedef struct MFMeshMaterial_s {
    float diffuse[3];
    float emission[3];
    float roughness;
    float metallic;

    const char* diffuse_texpath;
    const char* normal_texpath;
    const char* ao_texpath;
    const char* emission_texpath;
    const char* metallic_roughness_texpath;
    MFResourceSet* set; // TODO: Baked resource set manually by client for now. So change this later by using a resource set helper system
} MFMeshMaterial;

typedef struct MFMesh_s {
    MFGpuBuffer* vertBuffer;
    MFGpuBuffer* indBuffer;
    MFRenderer* renderer;

    u64 vertSize;
    u32 vertCount;

    MFVec3 localAABB[2];
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
