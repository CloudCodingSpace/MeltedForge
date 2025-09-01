#pragma once

#include "core/mfutils.h"
#include "mfcomponents.h"

typedef struct MFEntity_s {
    void* ownerScene;
    u64 uuid;
    u32 id;
    u32 compGrpId;
    u32 components;
} MFEntity;

MF_INLINE b8 mfEntityHasMeshComponent(const MFEntity* entity) {
    return (((entity->components & MF_COMPONENT_TYPE_MESH) == MF_COMPONENT_TYPE_MESH) ? true : false);
}

MF_INLINE b8 mfEntityHasTransformComponent(const MFEntity* entity) {
    return (((entity->components & MF_COMPONENT_TYPE_TRANSFORM) == MF_COMPONENT_TYPE_TRANSFORM) ? true : false);
}