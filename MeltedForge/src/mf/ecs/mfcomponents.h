#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfmaths.h"

#include "objects/mfmodel.h"

typedef enum MFComponentsType_e {
    MF_COMPONENT_TYPE_NONE,
    MF_COMPONENT_TYPE_MESH = 1 << 1,
    MF_COMPONENT_TYPE_TRANSFORM = 1 << 2
} MFComponentsType;

typedef struct MFTransformComponent_s {
    MFVec3 scale;
    MFVec3 position;
    MFVec3 rotationXYZ;
    b8 valid;
} MFTransformComponent;

typedef struct MFMeshComponent_s {
    MFModel model;
    const char* path;
    u64 perVertSize;
    MFModelVertexBuilder vertBuilder;
    b8 valid;
} MFMeshComponent;

typedef struct MFComponentGroup_s {
    u64 meshIdx;
    u64 transformIdx;
    b8 valid;
} MFComponentGroup;

#ifdef __cplusplus
}
#endif