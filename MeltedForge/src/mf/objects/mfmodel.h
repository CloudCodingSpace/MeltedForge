#pragma once

#include "mfmesh.h"
#include "core/mfutils.h"
#include "core/mfcore.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

typedef struct MFModelVertexBuilderData_s {
    MFVec3 pos;
    MFVec3 normal;
    MFVec3 tangent;
    MFVec2 texCoord;
} MFModelVertexBuilderData;

typedef void (*MFModelVertexBuilder)(void* dst, MFModelVertexBuilderData data);

typedef struct MFModel_s {
    u64 meshCount;
    MFMesh* meshes;
    b8 init;
} MFModel;

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder);
void mfModelDestroy(MFModel* model);