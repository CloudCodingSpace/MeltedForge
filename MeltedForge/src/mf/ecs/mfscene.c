#ifdef __cplusplus
extern "C" {
#endif

#include "mfscene.h"

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "ecs/mfcomponents.h"
#include "ecs/mfentity.h"

#include "objects/mfmodel.h"

#include "serializer/mfserializer.h"
#include "serializer/mfserializerutils.h"

#include <slog/slog.h>

void mfSceneCreate(MFScene* scene, MFCamera camera, MFModelVertexBuilder vertBuilder, MFRenderer* renderer) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(scene->init, mfGetLogger(), "The scene handle provided is already initialised!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle shouldn't be null!");
    MF_PANIC_IF(vertBuilder == mfnull, mfGetLogger(), "The vertex builder function pointer shouldn't be null!");
    
    scene->vertBuilder = vertBuilder;
    scene->camera = camera;
    scene->renderer = renderer;
    scene->entities = mfArrayCreate(4, sizeof(MFEntity));
    scene->meshCompPool = mfArrayCreate(4, sizeof(MFMeshComponent));
    scene->transformCompPool = mfArrayCreate(4, sizeof(MFTransformComponent));
    scene->compGrpTable = mfArrayCreate(4, sizeof(MFComponentGroup));
    scene->init = true;
}

void mfSceneDestroy(MFScene* scene) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");

    for(u32 i = 0; i < scene->entities.len; i++) {
        MFEntity* e = &mfArrayGetElement(scene->entities, MFEntity, i);
        if(!e->valid)
            continue;

        if(!mfEntityHasMeshComponent(e))
            continue;

        MFMeshComponent* comp = mfSceneEntityGetMeshComponent(scene, &e->id);
        mfModelDestroy(&comp->model);
    }
    
    mfArrayDestroy(&scene->entities);
    mfArrayDestroy(&scene->transformCompPool);
    mfArrayDestroy(&scene->meshCompPool);
    mfArrayDestroy(&scene->compGrpTable);

    mfCameraDestroy(&scene->camera);

    MF_SETMEM(scene, 0, sizeof(MFScene));
}

void mfSceneRender(MFScene* scene, MFSceneRenderConfig* config) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(config == mfnull, mfGetLogger(), "The scene render config function pointer shouldn't be null!");

    mfPipelineBind(config->entityPipeline, config->viewport, config->scissor);
    if(config->pipelineBindCallback)
        config->pipelineBindCallback(config->state, config->entityPipeline);

    for(u64 i = 0; i < scene->entities.len; i++) {
        MFEntity* e = &mfArrayGetElement(scene->entities, MFEntity, i);

        if(!e->valid)
            continue;

        if(!mfEntityHasMeshComponent(e) || !mfEntityHasTransformComponent(e))
            continue;
        
        MFMeshComponent* meshComp = mfSceneEntityGetMeshComponent(scene, &e->id);
        MFTransformComponent* transformComp = mfSceneEntityGetTransformComponent(scene, &e->id);

        MFMat4 modelMatrix = mfMat4Identity();
        if(config->computeModelMatrix)
            modelMatrix = config->computeModelMatrix(transformComp);

        for(u64 meshIdx = 0; meshIdx < meshComp->model.meshCount; meshIdx++) {
            MFMesh* mesh = &meshComp->model.meshes[meshIdx];
            if(config->perMeshDrawCallback)
                config->perMeshDrawCallback(config->state, mfMat4Mul(modelMatrix, mesh->transform), meshComp, meshIdx, config->entityPipeline);
            mfMeshRender(mesh);
        }
    }
}

//! IMPLEMENT THIS
void mfSceneUpdate(MFScene* scene) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
}

u64 mfSceneCreateEntity(MFScene* scene) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    
    MFComponentGroup grp = {};
    grp.valid = true;

    MFEntity entity = {};
    entity.compGrpId = scene->compGrpTable.len;
    entity.components = MF_COMPONENT_TYPE_NONE;
    entity.uuid = mfGetNextID();
    entity.id = scene->entities.len;
    entity.ownerScene = (void*)scene;
    entity.valid = true;

    mfArrayAddElement(&scene->compGrpTable, MFComponentGroup, grp);
    mfArrayAddElement(&scene->entities, MFEntity, entity);

    return scene->entities.len - 1;
}

void mfSceneDeleteEntity(MFScene* scene, u64* id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id > scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");

    MFEntity* e = &mfArrayGetElement(scene->entities, MFEntity, *id);

    if(e->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return;
    }

    if(mfEntityHasMeshComponent(e)) {
        MFMeshComponent* comp = mfSceneEntityGetMeshComponent(scene, id);
        mfModelDestroy(&comp->model);
        MF_SETMEM(comp, 0, sizeof(*comp));
    }

    if(mfEntityHasTransformComponent(e)) {
        MFTransformComponent* trans = mfSceneEntityGetTransformComponent(scene, id);
        MF_SETMEM(trans, 0, sizeof(*trans));
    }

    MFComponentGroup* grp = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, e->compGrpId);
    MF_SETMEM(grp, 0, sizeof(*grp));
    MF_SETMEM(e, 0, sizeof(MFEntity));

    *id = UINT64_MAX;
}

void mfSceneEntityAddMeshComponent(MFScene* scene, u64* id, MFMeshComponent comp) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(comp.path == mfnull, mfGetLogger(), "The path for the mesh component shouldn't be null!");
    MF_PANIC_IF(comp.perVertSize == 0, mfGetLogger(), "The perVertSize for the mesh component shouldn't be 0!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id > scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");
    
    MFEntity* entity = &mfArrayGetElement(scene->entities, MFEntity, *id);
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");
    
    if(entity->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return;
    }
    
    if(mfEntityHasMeshComponent(entity))
        return;
    
    comp.valid = true;
    mfArrayAddElement(&scene->meshCompPool, MFMeshComponent, comp);
    
    MFComponentGroup* grp = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->meshIdx = scene->meshCompPool.len - 1;

    MFMeshComponent* c = &mfArrayGetElement(scene->meshCompPool, MFMeshComponent, grp->meshIdx);
    mfModelLoadAndCreate(&c->model, comp.path, scene->renderer, comp.perVertSize, scene->vertBuilder);

    entity->components |= MF_COMPONENT_TYPE_MESH;
}

void mfSceneEntityAddTransformComponent(MFScene* scene, u64* id, MFTransformComponent comp) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id >= scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");

    MFEntity* entity = &mfArrayGetElement(scene->entities, MFEntity, *id);
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return;
    }

    if(mfEntityHasTransformComponent(entity))
        return;
   
    comp.valid = true;
    mfArrayAddElement(&scene->transformCompPool, MFTransformComponent, comp);
    MFComponentGroup* grp = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->transformIdx = scene->transformCompPool.len - 1;

    entity->components |= MF_COMPONENT_TYPE_TRANSFORM;
}

void mfSceneEntityRemoveMeshComponent(MFScene* scene, u64* id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id >= scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");

    MFEntity* entity = &mfArrayGetElement(scene->entities, MFEntity, *id);
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return;
    }

    if(!mfEntityHasMeshComponent(entity))
        return;

    MFMeshComponent* comp = mfSceneEntityGetMeshComponent(scene, id);
    mfModelDestroy(&comp->model);
    MF_SETMEM(comp, 0, sizeof(*comp));

    entity->components &= ~MF_COMPONENT_TYPE_MESH;
}

void mfSceneEntityRemoveTransformComponent(MFScene* scene, u64* id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id >= scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");
    
    MFEntity* entity = &mfArrayGetElement(scene->entities, MFEntity, *id);
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");
    
    if(entity->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return;
    }
    
    if(!mfEntityHasTransformComponent(entity))
        return;
    
    MFTransformComponent* comp = mfSceneEntityGetTransformComponent(scene, id);
    MF_SETMEM(comp, 0, sizeof(*comp));
    
    entity->components &= ~MF_COMPONENT_TYPE_TRANSFORM;
}

MFMeshComponent* mfSceneEntityGetMeshComponent(MFScene* scene, u64* id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id >= scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");

    MFEntity* entity = &mfArrayGetElement(scene->entities, MFEntity, *id);
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return mfnull;
    }

    if(!mfEntityHasMeshComponent(entity))
        return mfnull;

    MFComponentGroup* grp = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    MFMeshComponent* c = &mfArrayGetElement(scene->meshCompPool, MFMeshComponent, grp->meshIdx);
    return c;
}

MFTransformComponent* mfSceneEntityGetTransformComponent(MFScene* scene, u64* id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(id == mfnull, mfGetLogger(), "The entity id provided shouldn't be null!");
    MF_PANIC_IF(*id >= scene->entities.len, mfGetLogger(), "The entity's id provided isn't valid!");

    MFEntity* entity = &mfArrayGetElement(scene->entities, MFEntity, *id);
    MF_PANIC_IF(!entity->valid, mfGetLogger(), "The entity provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!");
        return mfnull;
    }

    if(!mfEntityHasTransformComponent(entity))
        return mfnull;

    MFComponentGroup* grp = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    MFTransformComponent* t = &mfArrayGetElement(scene->transformCompPool, MFTransformComponent, grp->transformIdx);

    return t;
}

void mfSceneGetValidEntities(MFScene* scene, u64* validEntityCount, MFEntity* entities) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(validEntityCount == mfnull, mfGetLogger(), "The pointer to valid entity count shouldn't be null!");
    
    *validEntityCount = 0;
    for(u64 i = 0; i < scene->entities.len; i++) {
        MFEntity* e = &mfArrayGetElement(scene->entities, MFEntity, i);
        if(e->valid) {
            if(entities)
                memcpy(&entities[*validEntityCount], e, sizeof(MFEntity));
            *validEntityCount = *validEntityCount + 1;
        }
    }
}

#ifdef __cplusplus
}
#endif