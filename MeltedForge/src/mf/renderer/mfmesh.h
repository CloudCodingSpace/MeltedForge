#pragma once

#include "mfrenderer.h"
#include "mfbuffer.h"

#include "core/mfutils.h"

typedef struct MFMesh_s {
    MFGpuBuffer* vertBuffer;
    MFGpuBuffer* indBuffer;
    MFRenderer* renderer;
    MFMat4 modelMat;

    u64 vertSize; // Size of the vertex buffer
    u32 vertCount; // Number of vertices including each triangle in this case equals the length of all the indices
} MFMesh;

void mfMeshCreate(MFMesh* mesh, MFRenderer* renderer, u64 vertSize, void* vertices, u32 indCount, u32* indices);
void mfMeshDestroy(MFMesh* mesh);

void mfMeshRender(MFMesh* mesh);