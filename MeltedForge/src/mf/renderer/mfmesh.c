#include "mfmesh.h"

void mfMeshCreate(MFMesh* mesh, MFRenderer* renderer, u64 vertSize, void* vertices, u32 indCount, u32* indices) {
    MF_ASSERT(mesh == mfnull, mfGetLogger(), "The mesh handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_ASSERT(vertices == mfnull, mfGetLogger(), "The vertices provided shouldn't be null!");
    MF_ASSERT(indices == mfnull, mfGetLogger(), "The indices provided shouldn't be null!");

    mesh->vertSize = vertSize;
    mesh->vertCount = indCount;
    mesh->renderer = renderer;

    mesh->vertBuffer = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
    mesh->indBuffer = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());

    MFGpuBufferConfig config = {
        .data = vertices,
        .size = vertSize,
        .type = MF_GPU_BUFFER_TYPE_VERTEX
    };

    mfGpuBufferAllocate(mesh->vertBuffer, config, renderer);
    
    config.data = indices;
    config.size = sizeof(u32) * indCount;
    config.type = MF_GPU_BUFFER_TYPE_INDEX;
    
    mfGpuBufferAllocate(mesh->indBuffer, config, renderer);
}

void mfMeshDestroy(MFMesh* mesh) {
    MF_ASSERT(mesh == mfnull, mfGetLogger(), "The mesh handle provided shouldn't be null!");
    
    mfGpuBufferFree(mesh->indBuffer);
    mfGpuBufferFree(mesh->vertBuffer);
    
    MF_FREEMEM(mesh->vertBuffer);
    MF_FREEMEM(mesh->indBuffer);

    MF_SETMEM(mesh, 0, sizeof(MFMesh));
}

void mfMeshRender(MFMesh* mesh) {
    MF_ASSERT(mesh == mfnull, mfGetLogger(), "The mesh handle provided shouldn't be null!");

    mfGpuBufferBind(mesh->vertBuffer);
    mfGpuBufferBind(mesh->indBuffer);
    mfRendererDrawVerticesIndexed(mesh->renderer, mesh->vertCount, 1, 0, 0);
}