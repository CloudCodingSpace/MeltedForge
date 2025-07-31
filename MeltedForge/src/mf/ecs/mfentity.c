#include "mfentity.h"

void mfEntityCreate(MFEntity* entity) {
    MF_ASSERT(entity == mfnull, mfGetLogger(), "The entity handle shouldn't be null!");
    
    MF_SETMEM(entity, 0, sizeof(MFEntity));
    // TODO: Find a way to get an UUID
}

void mfEntityDestroy(MFEntity* entity) {
    MF_ASSERT(entity == mfnull, mfGetLogger(), "The entity handle shouldn't be null!");
    if(entity->transform) {
        MF_FREEMEM(entity->transform);
    }
    if(entity->mesh) {
        if(entity->mesh->path) {
            MF_FREEMEM(entity->mesh->path);
        }
        MF_FREEMEM(entity->mesh);
    }

    MF_SETMEM(entity, 0, sizeof(MFEntity));
}

void mfEntityCreateTransformComponent(MFEntity* entity, MFVec3 scale, MFVec3 pos, MFVec3 rotation) {
    MF_ASSERT(entity == mfnull, mfGetLogger(), "The entity handle shouldn't be null!");

    if(entity->transform)
        return;
    entity->transform = MF_ALLOCMEM(MFTransformComponent, sizeof(MFTransformComponent));
    entity->transform->scale = scale;
    entity->transform->position = pos;
    entity->transform->rotationXYZ = rotation;
}

void mfEntityCreateMeshComponent(MFEntity* entity, const char* path) {
    MF_ASSERT(entity == mfnull, mfGetLogger(), "The entity handle shouldn't be null!");
    MF_ASSERT(path == mfnull, mfGetLogger(), "The path provided shoudln't be null!");
    
    if(entity->mesh)
        return;
    entity->mesh = MF_ALLOCMEM(MFMeshComponent, sizeof(MFMeshComponent));
    entity->mesh->path = mfStringDuplicate(mfGetLogger(), path);
}