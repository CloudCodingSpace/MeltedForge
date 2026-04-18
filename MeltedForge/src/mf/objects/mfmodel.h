#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mfmesh.h"
#include "core/mfutils.h"
#include "core/mfcore.h"

typedef struct MFModelVertexBuilderData_s {
    MFVec3 pos;
    MFVec3 normal;
    MFVec3 tangent;
    MFVec2 texCoord;
} MFModelVertexBuilderData;

typedef void (*MFModelVertexBuilder)(void* dst, MFModelVertexBuilderData data);

typedef struct MFModel_s {
    u64 meshCount, perVertexSize, _meshIdx;
    MFMesh* meshes;

    MFModelVertexBuilder builder;
    MFRenderer* renderer;
    
    bool init;
} MFModel;

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder);
void mfModelDestroy(MFModel* model);

#ifdef __cplusplus
}
#endif