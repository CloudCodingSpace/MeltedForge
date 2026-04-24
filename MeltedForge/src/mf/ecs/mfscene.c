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

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(scene->init, mfGetLogger(), "The scene handle provided is already initialised!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle shouldn't be null!");
    
    scene->camera = camera;
    scene->renderer = renderer;
    scene->entities = mfArrayCreate(mfGetLogger(), 4, sizeof(MFEntity));
    scene->meshCompPool = mfArrayCreate(mfGetLogger(), 4, sizeof(MFMeshComponent));
    scene->transformCompPool = mfArrayCreate(mfGetLogger(), 4, sizeof(MFTransformComponent));
    scene->compGrpTable = mfArrayCreate(mfGetLogger(), 4, sizeof(MFComponentGroup));
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
    
    mfArrayDestroy(&scene->entities, mfGetLogger());
    mfArrayDestroy(&scene->transformCompPool, mfGetLogger());
    mfArrayDestroy(&scene->meshCompPool, mfGetLogger());
    mfArrayDestroy(&scene->compGrpTable, mfGetLogger());

    mfCameraDestroy(&scene->camera);

    MF_SETMEM(scene, 0, sizeof(MFScene));
}

void mfSceneRender(MFScene* scene, MFSceneRenderConfig* config) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(config == mfnull, mfGetLogger(), "The scene render config function pointer shouldn't be null!");

    mfPipelineBind(config->entityPipeline, config->viewport, config->scissor);
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

    mfArrayAddElement(scene->compGrpTable, MFComponentGroup, mfGetLogger(), grp);
    mfArrayAddElement(scene->entities, MFEntity, mfGetLogger(), entity);

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
    MF_PANIC_IF(comp.vertBuilder == mfnull, mfGetLogger(), "The vertBuilder for the mesh component shouldn't be null!");
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
    mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), comp);
    
    MFComponentGroup* grp = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->meshIdx = scene->meshCompPool.len - 1;

    MFMeshComponent* c = &mfArrayGetElement(scene->meshCompPool, MFMeshComponent, grp->meshIdx);
    mfModelLoadAndCreate(&c->model, comp.path, scene->renderer, comp.perVertSize, comp.vertBuilder);

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
    mfArrayAddElement(scene->transformCompPool, MFTransformComponent, mfGetLogger(), comp);
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

    entity->components &= MF_COMPONENT_TYPE_MESH;
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
    
    entity->components &= MF_COMPONENT_TYPE_TRANSFORM;
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

void mfSceneSerialize(MFScene* scene, const char* fileName) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(fileName == mfnull, mfGetLogger(), "The file name shouldn't be null!");

    u64 size = sizeof(u32); // For the scene signature
    size += sizeof(u64) * 4; // For the sizes of the 4 mfarrays

    u64 validEntities = 0, validMeshComponents = 0, validTransformComponents = 0, validGroupCount = 0;
    for(u64 i = 0; i < scene->entities.len; i++) {
        MFEntity* e = &mfArrayGetElement(scene->entities, MFEntity, i);
        if(e->valid)
            validEntities++;
    }
    for(u64 i = 0; i < scene->meshCompPool.len; i++) {
        MFMeshComponent* c = &mfArrayGetElement(scene->meshCompPool, MFMeshComponent, i);
        if(c->valid)
            validMeshComponents++;
    }
    for(u64 i = 0; i < scene->transformCompPool.len; i++) {
        MFTransformComponent* t = &mfArrayGetElement(scene->transformCompPool, MFTransformComponent, i);
        if(t->valid)
            validTransformComponents++;
    }
    for(u64 i = 0; i < scene->compGrpTable.len; i++) {
        MFComponentGroup* g = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, i);
        if(g->valid)
            validGroupCount++;
    }

    size += validEntities * sizeof(u64);
    size += validTransformComponents * sizeof(u64);
    size += validGroupCount * sizeof(u64);
    size += validMeshComponents * sizeof(u64);

    size += 4 * sizeof(u64) * validEntities; // since 4 u64s per entity
    size += 9 * sizeof(f32) * validTransformComponents; // since 9 f32s per transform component
    size += 2 * sizeof(u64) * validGroupCount; // since 2 u64s per group

    for(u64 i = 0; i < scene->meshCompPool.len; i++) {
        MFMeshComponent* comp = &mfArrayGetElement(scene->meshCompPool, MFMeshComponent, i);
        if(!comp)
            continue;
        size += sizeof(u32);
        size += sizeof(char) * mfStringLen(comp->path);
    }

    // Camera size
    size += sizeof(f32) * 3 * 4;
    size += sizeof(f32) * 7;

    MFSerializer s = {};
    mfSerializerCreate(&s, size, false);

    mfSerializeU32(&s, MF_SIGNATURE_SCENE_FILE);

    // Camera
    {
        MFCamera* camera = &scene->camera;

        mfSerializeF32(&s, camera->front.x);
        mfSerializeF32(&s, camera->front.y);
        mfSerializeF32(&s, camera->front.z);

        mfSerializeF32(&s, camera->up.x);
        mfSerializeF32(&s, camera->up.y);
        mfSerializeF32(&s, camera->up.z);

        mfSerializeF32(&s, camera->right.x);
        mfSerializeF32(&s, camera->right.y);
        mfSerializeF32(&s, camera->right.z);

        mfSerializeF32(&s, camera->pos.x);
        mfSerializeF32(&s, camera->pos.y);
        mfSerializeF32(&s, camera->pos.z);
    
        mfSerializeF32(&s, camera->yaw);
        mfSerializeF32(&s, camera->pitch);
        mfSerializeF32(&s, camera->fov);
        mfSerializeF32(&s, camera->nearPlane);
        mfSerializeF32(&s, camera->farPlane);
        mfSerializeF32(&s, camera->speed);
        mfSerializeF32(&s, camera->sensitivity);
    }

    // Entity array 
    {
        mfSerializeU64(&s, validEntities);
        for(u64 i = 0; i < scene->entities.len; i++) {
            MFEntity* e = &mfArrayGetElement(scene->entities, MFEntity, i);
            if(!e->valid)
                continue;
            mfSerializeU64(&s, e->uuid);
            mfSerializeU64(&s, e->id);
            mfSerializeU64(&s, e->compGrpId);
            mfSerializeU64(&s, e->components);
        }
    }
    // Mesh comp array 
    {
        mfSerializeU64(&s, validMeshComponents);
        for(u64 i = 0; i < scene->meshCompPool.len; i++) {
            MFMeshComponent* c = &mfArrayGetElement(scene->meshCompPool, MFMeshComponent, i);
            if(!c->valid)
                continue;
            mfSerializeString(&s, c->path);
            mfSerializeU64(&s, c->perVertSize);
        }
    }
    // Transform comp array 
    {
        mfSerializeU64(&s, validTransformComponents);
        for(u64 i = 0; i < scene->transformCompPool.len; i++) {
            MFTransformComponent* t = &mfArrayGetElement(scene->transformCompPool, MFTransformComponent, i);
            if(!t->valid)
                continue;
            mfSerializeF32(&s, t->position.x);
            mfSerializeF32(&s, t->position.y);
            mfSerializeF32(&s, t->position.z);
            mfSerializeF32(&s, t->rotationXYZ.x);
            mfSerializeF32(&s, t->rotationXYZ.y);
            mfSerializeF32(&s, t->rotationXYZ.z);
            mfSerializeF32(&s, t->scale.x);
            mfSerializeF32(&s, t->scale.y);
            mfSerializeF32(&s, t->scale.z);
        }
    }
    // Component group array 
    {
        mfSerializeU64(&s, validGroupCount);
        for(u64 i = 0; i < scene->compGrpTable.len; i++) {
            MFComponentGroup* g = &mfArrayGetElement(scene->compGrpTable, MFComponentGroup, i);
            if(!g->valid)
                continue;
            mfSerializeU64(&s, g->meshIdx);
            mfSerializeU64(&s, g->transformIdx);
        }
    }

    bool success = mfWriteFile(mfGetLogger(), s.bufferSize, fileName, (const char*)s.buffer, "wb");
    if(!success) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't write to file '%s'!", fileName);
    }

    mfSerializerDestroy(&s);
}

bool mfSceneDeserialize(MFScene* scene, const char* fileName, MFModelVertexBuilder vertexBuilder) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(!scene->init, mfGetLogger(), "The scene handle provided isn't initialised!");
    MF_PANIC_IF(fileName == mfnull, mfGetLogger(), "The file name shouldn't be null!");

    u64 fileSize = 0;
    bool success = false;
    u8* content = mfReadFile(mfGetLogger(), &fileSize, &success, fileName, "rb");
    if(!success) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to open file '%s'!", fileName);
        return false;
    }

    MFSerializer s = {
        .buffer = content,
    };
    mfSerializerCreate(&s, fileSize, true);

    u32 sign = mfDeserializeU32(&s);
    if(sign != MF_SIGNATURE_SCENE_FILE) {
        MF_FREEMEM(content);
        return false;
    }

    mfArrayDestroy(&scene->entities, mfGetLogger());
    mfArrayDestroy(&scene->compGrpTable, mfGetLogger());
    mfArrayDestroy(&scene->meshCompPool, mfGetLogger());
    mfArrayDestroy(&scene->transformCompPool, mfGetLogger());

    // Camera
    {
        MFCamera* camera = &scene->camera;

        camera->front.x = mfDeserializeF32(&s);
        camera->front.y = mfDeserializeF32(&s);
        camera->front.z = mfDeserializeF32(&s);

        camera->up.x = mfDeserializeF32(&s);
        camera->up.y = mfDeserializeF32(&s);
        camera->up.z = mfDeserializeF32(&s);
        
        camera->right.x = mfDeserializeF32(&s);
        camera->right.y = mfDeserializeF32(&s);
        camera->right.z = mfDeserializeF32(&s);
        
        camera->pos.x = mfDeserializeF32(&s);
        camera->pos.y = mfDeserializeF32(&s);
        camera->pos.z = mfDeserializeF32(&s);

        camera->yaw = mfDeserializeF32(&s);
        camera->pitch = mfDeserializeF32(&s);
        camera->fov = mfDeserializeF32(&s);
        camera->nearPlane = mfDeserializeF32(&s);
        camera->farPlane = mfDeserializeF32(&s);
        camera->speed = mfDeserializeF32(&s);
        camera->sensitivity = mfDeserializeF32(&s);

        camera->constructMatrices(camera);
    }

    u64 eLen = mfDeserializeU64(&s);
    scene->entities = mfArrayCreate(mfGetLogger(), eLen, sizeof(MFEntity));
    for(u64 i = 0; i < eLen; i++) {
        MFEntity e = {0};
        e.ownerScene = scene;
        e.valid = true;
        e.uuid = mfDeserializeU64(&s);
        e.id = mfDeserializeU64(&s);
        e.compGrpId = mfDeserializeU64(&s);
        e.components = mfDeserializeU64(&s);
        mfArrayAddElement(scene->entities, MFEntity, mfGetLogger(), e);
    }

    u64 mLen = mfDeserializeU64(&s);
    scene->meshCompPool = mfArrayCreate(mfGetLogger(), mLen, sizeof(MFMeshComponent));

    for(u64 i = 0; i < mLen; i++) {
        MFMeshComponent c = {0};
        c.path = mfDeserializeString(&s);
        c.perVertSize = mfDeserializeU64(&s);
        c.vertBuilder = vertexBuilder;
        c.valid = true;
        mfModelLoadAndCreate(&c.model, c.path, scene->renderer, c.perVertSize, c.vertBuilder);
        mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), c);
    }

    u64 tLen = mfDeserializeU64(&s);
    scene->transformCompPool = mfArrayCreate(mfGetLogger(), tLen, sizeof(MFTransformComponent));
    for(u64 i = 0; i < tLen; i++) {
        MFTransformComponent t = {0};
        t.valid = true;
        t.position.x = mfDeserializeF32(&s);
        t.position.y = mfDeserializeF32(&s);
        t.position.z = mfDeserializeF32(&s);
        t.rotationXYZ.x = mfDeserializeF32(&s);
        t.rotationXYZ.y = mfDeserializeF32(&s);
        t.rotationXYZ.z = mfDeserializeF32(&s);
        t.scale.x = mfDeserializeF32(&s);
        t.scale.y = mfDeserializeF32(&s);
        t.scale.z = mfDeserializeF32(&s);
        mfArrayAddElement(scene->transformCompPool, MFTransformComponent, mfGetLogger(), t);
    }

    u64 gLen = mfDeserializeU64(&s);
    scene->compGrpTable = mfArrayCreate(mfGetLogger(), gLen, sizeof(MFComponentGroup));
    for(u64 i = 0; i < gLen; i++) {
        MFComponentGroup g = {0};
        g.valid = true;
        g.meshIdx = mfDeserializeU64(&s);
        g.transformIdx = mfDeserializeU64(&s);
        mfArrayAddElement(scene->compGrpTable, MFComponentGroup, mfGetLogger(), g);
    }

    mfSerializerDestroy(&s);

    scene->init = true;

    return true;
}

#ifdef __cplusplus
}
#endif