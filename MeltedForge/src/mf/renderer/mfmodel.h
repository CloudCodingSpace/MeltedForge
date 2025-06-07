#pragma once

#include "mfmesh.h"
#include "core/mfutils.h"
#include "core/mfcore.h"

#include <tinyobj/tinyobjloader_c.h>

typedef void (*MFModelVertexBuilder)(void* dst, const tinyobj_attrib_t* attrib, const tinyobj_vertex_index_t* idx);

typedef struct MFModel_s {
    u64 meshCount;
    MFMesh* meshes;
} MFModel;

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder);
void mfModelDestroy(MFModel* model);

void mfModelRender(MFModel* model);