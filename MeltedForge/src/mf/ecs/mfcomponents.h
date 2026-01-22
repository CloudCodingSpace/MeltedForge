#pragma once

#include "core/mfmaths.h"

#include "renderer/mfmodel.h"

// TODO: Add more components when necessary
typedef enum MFComponentsType_e {
    MF_COMPONENT_TYPE_NONE,
    MF_COMPONENT_TYPE_MESH,
    MF_COMPONENT_TYPE_TRANSFORM
} MFComponentsType;

typedef struct MFTransformComponent_s {
    MFVec3 scale;
    MFVec3 position;
    MFVec3 rotationXYZ;
} MFTransformComponent;

typedef struct MFMeshComponent_s {
    MFModel model;
    const char* path;
    u64 perVertSize;
    MFModelVertexBuilder vertBuilder;
} MFMeshComponent;

typedef struct MFComponentGroup_s {
    u64 meshIdx;
    u64 transformIdx;
} MFComponentGroup;
