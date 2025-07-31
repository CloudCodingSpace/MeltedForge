#pragma once

#include "mfcomponents.h"
#include "core/mfutils.h"

typedef struct MFEntity_s {
    MFTransformComponent* transform;
    MFMeshComponent* mesh;
    u64 uuid;
} MFEntity;

void mfEntityCreate(MFEntity* entity);
void mfEntityDestroy(MFEntity* entity);

void mfEntityCreateTransformComponent(MFEntity* entity, MFVec3 scale, MFVec3 pos, MFVec3 rotation);
void mfEntityCreateMeshComponent(MFEntity* entity, const char* path);