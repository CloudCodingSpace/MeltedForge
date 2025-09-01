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

    for(u32 i = 0; i < scene->entities.len; i++) {
        MFEntity* e = &mfArrayGet(scene->entities, MFEntity, i);
        if(e->id == UINT_MAX)
            continue;
        
        if(!mfEntityHasMeshComponent(e))
            continue;

        MFMeshComponent* comp = mfSceneEntityGetMeshComponent(scene, e->id);
        mfModelDestroy(&comp->model);
    }
    
    mfArrayDestroy(&scene->entities, mfGetLogger());
    mfArrayDestroy(&scene->transformCompPool, mfGetLogger());
    mfArrayDestroy(&scene->meshCompPool, mfGetLogger());
    mfArrayDestroy(&scene->compGrpTable, mfGetLogger());
}

void mfSceneRender(MFScene* scene, void (*entityDraw)(MFEntity* e, MFScene* scene, void* state), void* state) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(entityDraw == mfnull, mfGetLogger(), "The entity draw function ptr handle shouldn't be null!");
    
    for(u64 i = 0; i < scene->entities.len; i++) {
        MFEntity* e = &mfArrayGet(scene->entities, MFEntity, i);
        if(e->id == UINT_MAX)
            continue;

        if(!mfEntityHasMeshComponent(e) || !mfEntityHasTransformComponent(e))
            continue;
        
        entityDraw(e, scene, state);
    }
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

    mfArrayAddElement(scene->compGrpTable, MFComponentGroup, mfGetLogger(), grp);
    mfArrayAddElement(scene->entities, MFEntity, mfGetLogger(), entity);

    MFEntity* e = &mfArrayGet(scene->entities, MFEntity, scene->entities.len - 1);

    return e;
}

void mfSceneDeleteEntity(MFScene* scene, MFEntity* e) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(e == mfnull, mfGetLogger(), "The entity handle shouldn't be null!"); 

    if(e->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }

    if(mfEntityHasMeshComponent(e)) {
        MFMeshComponent* comp = mfSceneEntityGetMeshComponent(scene, e->id);
        mfModelDestroy(&comp->model);
        
        MF_SETMEM(comp, 0, sizeof(*comp));
    }
    if(mfEntityHasTransformComponent(e)) {
        MFTransformComponent* trans = mfSceneEntityGetTransformComponent(scene, e->id);
        MF_SETMEM(trans, 0, sizeof(*trans));
    }
    
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, e->compGrpId);
    
    scene->compGrpTable.len--;
    scene->entities.len--;

    MF_SETMEM(grp, 0, sizeof(*grp));
    MF_SETMEM(e, 0, sizeof(*e));

    e->id = UINT_MAX;
}

void mfSceneEntityAddMeshComponent(MFScene* scene, u32 id, MFMeshComponent comp) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(comp.path == mfnull, mfGetLogger(), "The path for the mesh component shouldn't be null!");
    MF_ASSERT(comp.perVertSize == 0, mfGetLogger(), "The perVertSize for the mesh component shouldn't be 0!");
    MF_ASSERT(comp.vertBuilder == mfnull, mfGetLogger(), "The vertBuilder for the mesh component shouldn't be null!");
    MF_ASSERT(id > scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");
    
    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_ASSERT(entity->id == UINT_MAX, mfGetLogger(), "The entity provided isn't valid anymore!");
    
    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }
    
    if(mfEntityHasMeshComponent(entity))
        return;
    
    mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), comp);
    
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->mesh = &mfArrayGet(scene->meshCompPool, MFMeshComponent, scene->meshCompPool.len - 1);

    mfModelLoadAndCreate(&grp->mesh->model, comp.path, scene->renderer, comp.perVertSize, comp.vertBuilder);

    entity->components |= MF_COMPONENT_TYPE_MESH;
}

void mfSceneEntityAddTransformComponent(MFScene* scene, u32 id, MFTransformComponent comp) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_ASSERT(entity->id == UINT_MAX, mfGetLogger(), "The entity's id provided isn't valid anymore!");

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

MFMeshComponent* mfSceneEntityGetMeshComponent(MFScene* scene, u32 id) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_ASSERT(entity->id == UINT_MAX, mfGetLogger(), "The entity's id provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return mfnull;
    }

    if(!mfEntityHasMeshComponent(entity))
        return mfnull;

    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    return grp->mesh;
}

MFTransformComponent* mfSceneEntityGetTransformComponent(MFScene* scene, u32 id) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_ASSERT(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_ASSERT(entity->id == UINT_MAX, mfGetLogger(), "The entity's id provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return mfnull;
    }

    if(!mfEntityHasTransformComponent(entity))
        return mfnull;

    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    return grp->transform;
}