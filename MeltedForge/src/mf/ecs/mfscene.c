#include "mfscene.h"

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle shouldn't be null!");
    
    scene->camera = camera;
    scene->renderer = renderer;
    scene->entities = mfArrayCreate(mfGetLogger(), 2, sizeof(MFEntity));
    scene->meshCompPool = mfArrayCreate(mfGetLogger(), 2, sizeof(MFMeshComponent));
    scene->transformCompPool = mfArrayCreate(mfGetLogger(), 2, sizeof(MFTransformComponent));
    scene->compGrpTable = mfArrayCreate(mfGetLogger(), 2, sizeof(MFComponentGroup));
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

MFEntity* mfSceneCreateEntity(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
    MFComponentGroup grp = {};

    MFEntity entity = {};
    entity.components = MF_COMPONENT_TYPE_NONE;
    entity.uuid = 0xffffffff; // TODO: Find a way to generate UUIDs!!
    entity.id = scene->compGrpTable.len;
    entity.ownerScene = (void*)scene;

    mfArrayAddElement(scene->compGrpTable, MFComponentGroup, mfGetLogger(), grp);
    mfArrayAddElement(scene->entities, MFEntity, mfGetLogger(), entity);

    return &mfArrayGet(scene->entities, MFEntity, scene->entities.len - 1);
}

//! Find a way to identify if the entity belongs to this scene!
void mfSceneEntityAddMeshComponent(MFScene* scene, MFEntity* entity, MFMeshComponent comp) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(entity == mfnull, mfGetLogger(), "The entity handle shouldn't be null!");
 
    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }

    if((entity->components & MF_COMPONENT_TYPE_MESH) == MF_COMPONENT_TYPE_MESH)
        return;
    
    mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), comp);

    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->id);
    grp->mesh = &mfArrayGet(scene->meshCompPool, MFMeshComponent, scene->meshCompPool.len - 1);

    entity->components |= MF_COMPONENT_TYPE_MESH;
}

//! Find a way to identify if the entity belongs to this scene!
void mfSceneEntityAddTransformComponent(MFScene* scene, MFEntity* entity, MFTransformComponent comp) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(entity == mfnull, mfGetLogger(), "The entity handle shouldn't be null!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }

    if((entity->components & MF_COMPONENT_TYPE_TRANSFORM) == MF_COMPONENT_TYPE_TRANSFORM)
        return;
    
    mfArrayAddElement(scene->transformCompPool, MFTransformComponent, mfGetLogger(), comp);
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->id);
    grp->transform = &mfArrayGet(scene->transformCompPool, MFTransformComponent, scene->transformCompPool.len - 1);

    entity->components |= MF_COMPONENT_TYPE_TRANSFORM;
}