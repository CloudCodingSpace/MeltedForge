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
    MFVec3 bitangent;
    MFVec2 texCoord;
    MFVec4 tangent;
    MFVec3 color;
} MFModelVertexBuilderData;

typedef void (*MFModelVertexBuilder)(void* dst, MFModelVertexBuilderData data);

typedef struct MFModel_s {
    u64 meshCount, perVertexSize, _meshIdx;
    MFMesh* meshes;

    MFModelVertexBuilder builder;
    MFRenderer* renderer;
    
    bool init;
} MFModel;

// @brief Loads a model file from disk and creates it
// @param model A valid pointer to a `MFModel` object
// @param filePath A valid const char* indicating the path to the model in disk
// @param renderer A valid pointer to a MFRenderer
// @param perVertSize Size of each vertex data in bytes
// @param builder A valid function pointer which builds the user's vertex data based on the available data
void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder);

// @brief Destroys a model
// @param model A valid pointer to a `MFModel` object
void mfModelDestroy(MFModel* model);

#ifdef __cplusplus
}
#endif