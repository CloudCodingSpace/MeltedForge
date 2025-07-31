#pragma once

#include "core/mfmaths.h"

#include "renderer/mfmodel.h"

typedef struct MFTransformComponent_s {
    MFVec3 scale;
    MFVec3 position;
    MFVec3 rotationXYZ;
} MFTransformComponent;

typedef struct MFMeshComponent_s {
    MFModel model;
    const char* path;
} MFMeshComponent;

// TODO: Add more components when necessary