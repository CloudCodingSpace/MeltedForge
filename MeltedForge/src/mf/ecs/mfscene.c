#include "mfscene.h"

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle shouldn't be null!");
    
    scene->camera = camera;
    scene->renderer = renderer;
    scene->entities = mfArrayCreate(mfGetLogger(), 4, sizeof(MFEntity));
    scene->meshCompPool = mfArrayCreate(mfGetLogger(), 4, sizeof(MFMeshComponent));
    scene->transformCompPool = mfArrayCreate(mfGetLogger(), 4, sizeof(MFTransformComponent));
    scene->compGrpTable = mfArrayCreate(mfGetLogger(), 4, sizeof(MFComponentGroup));
}

void mfSceneDestroy(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
    mfArrayDestroy(&scene->entities, mfGetLogger());
    mfArrayDestroy(&scene->transformCompPool, mfGetLogger());
    mfArrayDestroy(&scene->meshCompPool, mfGetLogger());
    mfArrayDestroy(&scene->compGrpTable, mfGetLogger());
}

//! IMPLEMENT THIS
void mfSceneRender(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
}

//! IMPLEMENT THIS
void mfSceneUpdate(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
}

const MFEntity* mfSceneCreateEntity(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
    MFComponentGroup grp = {};

    MFEntity entity = {};
    entity.compGrpId = scene->compGrpTable.len;
    entity.components = MF_COMPONENT_TYPE_NONE;
    entity.uuid = 0xffffffff; // TODO: Find a way to generate UUIDs!!
    entity.id = scene->entities.len;
    entity.ownerScene = (void*)scene;
    entity.valid = true;

    mfArrayAddElement(scene->compGrpTable, MFComponentGroup, mfGetLogger(), grp);
    mfArrayAddElement(scene->entities, MFEntity, mfGetLogger(), entity);

    MFEntity* e = &mfArrayGet(scene->entities, MFEntity, scene->entities.len - 1);

    return e;
}

void mfSceneEntityAddMeshComponent(MFScene* scene, u32 id, MFMeshComponent comp) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(id > scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");
    
    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_ASSERT(entity->valid != true, mfGetLogger(), "The entity provided isn't valid anymore!");
    
    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }
    
    if(mfEntityHasMeshComponent(entity))
        return;
    
    mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), comp);
    
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->mesh = &mfArrayGet(scene->meshCompPool, MFMeshComponent, scene->meshCompPool.len - 1);
    
    entity->components |= MF_COMPONENT_TYPE_MESH;
}

void mfSceneEntityAddTransformComponent(MFScene* scene, u32 id, MFTransformComponent comp) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_ASSERT(entity->valid != true, mfGetLogger(), "The entity's id provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }

    if(mfEntityHasTransformComponent(entity))
        return;
    
    mfArrayAddElement(scene->transformCompPool, MFTransformComponent, mfGetLogger(), comp);
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->transform = &mfArrayGet(scene->transformCompPool, MFTransformComponent, scene->transformCompPool.len - 1);

    entity->components |= MF_COMPONENT_TYPE_TRANSFORM;
}