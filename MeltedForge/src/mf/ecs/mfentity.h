#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "mfcomponents.h"

typedef struct MFEntity_s {
    void* ownerScene;
    u64 uuid;
    u64 id;
    u64 compGrpId;
    u64 components;
    bool valid;
} MFEntity;

MF_INLINE bool mfEntityHasMeshComponent(const MFEntity* entity) {
    MF_PANIC_IF(entity == mfnull, mfGetLogger(), "The entity provided shouldn't be null!");
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");

    return entity->components & MF_COMPONENT_TYPE_MESH;
}

MF_INLINE bool mfEntityHasTransformComponent(const MFEntity* entity) {
    MF_PANIC_IF(entity == mfnull, mfGetLogger(), "The entity provided shouldn't be null!");
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");

    return entity->components & MF_COMPONENT_TYPE_TRANSFORM;
}

#ifdef __cplusplus
}
#endif