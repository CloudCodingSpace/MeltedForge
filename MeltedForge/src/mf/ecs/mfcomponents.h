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
} MFMeshComponent;

typedef struct MFComponentGroup_s {
    MFMeshComponent* mesh;
    MFTransformComponent* transform;
} MFComponentGroup;