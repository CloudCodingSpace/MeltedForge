#include "mfscene.h"

#include "core/mfcore.h"
#include "core/mfutils.h"
#include "ecs/mfcomponents.h"
#include "ecs/mfentity.h"
#include "renderer/mfmesh.h"
#include "renderer/mfmodel.h"
#include "serializer/mfserializer.h"
#include "serializer/mfserializerutils.h"
#include "slog/slog.h"

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle shouldn't be null!");
    
    scene->camera = camera;
    scene->renderer = renderer;
    scene->entities = mfArrayCreate(mfGetLogger(), 4, sizeof(MFEntity));
    scene->meshCompPool = mfArrayCreate(mfGetLogger(), 4, sizeof(MFMeshComponent));
    scene->transformCompPool = mfArrayCreate(mfGetLogger(), 4, sizeof(MFTransformComponent));
    scene->compGrpTable = mfArrayCreate(mfGetLogger(), 4, sizeof(MFComponentGroup));
}

void mfSceneDestroy(MFScene* scene) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");

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
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(entityDraw == mfnull, mfGetLogger(), "The entity draw function ptr handle shouldn't be null!");
    
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
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
}

const MFEntity* mfSceneCreateEntity(MFScene* scene) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    
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
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(e == mfnull, mfGetLogger(), "The entity handle shouldn't be null!"); 

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
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(comp.path == mfnull, mfGetLogger(), "The path for the mesh component shouldn't be null!");
    MF_PANIC_IF(comp.perVertSize == 0, mfGetLogger(), "The perVertSize for the mesh component shouldn't be 0!");
    MF_PANIC_IF(comp.vertBuilder == mfnull, mfGetLogger(), "The vertBuilder for the mesh component shouldn't be null!");
    MF_PANIC_IF(id > scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");
    
    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_PANIC_IF(entity->id == UINT_MAX, mfGetLogger(), "The entity provided isn't valid anymore!");
    
    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }
    
    if(mfEntityHasMeshComponent(entity))
        return;
    
    mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), comp);
    
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->meshIdx = scene->meshCompPool.len - 1;

    MFMeshComponent* c = &mfArrayGet(scene->meshCompPool, MFMeshComponent, grp->meshIdx);
    mfModelLoadAndCreate(&c->model, comp.path, scene->renderer, comp.perVertSize, comp.vertBuilder);

    entity->components |= MF_COMPONENT_TYPE_MESH;
}

void mfSceneEntityAddTransformComponent(MFScene* scene, u32 id, MFTransformComponent comp) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_PANIC_IF(entity->id == UINT_MAX, mfGetLogger(), "The entity's id provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return;
    }

    if(mfEntityHasTransformComponent(entity))
        return;
    
    mfArrayAddElement(scene->transformCompPool, MFTransformComponent, mfGetLogger(), comp);
    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    grp->transformIdx = scene->transformCompPool.len - 1;

    entity->components |= MF_COMPONENT_TYPE_TRANSFORM;
}

MFMeshComponent* mfSceneEntityGetMeshComponent(MFScene* scene, u32 id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_PANIC_IF(entity->id == UINT_MAX, mfGetLogger(), "The entity's id provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return mfnull;
    }

    if(!mfEntityHasMeshComponent(entity))
        return mfnull;

    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    MFMeshComponent* c = &mfArrayGet(scene->meshCompPool, MFMeshComponent, grp->meshIdx);
    return c;
}

MFTransformComponent* mfSceneEntityGetTransformComponent(MFScene* scene, u32 id) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(id >= scene->entities.len, mfGetLogger(), "The entity's id provided, isn't valid!");

    MFEntity* entity = &mfArrayGet(scene->entities, MFEntity, id);
    MF_PANIC_IF(entity->id == UINT_MAX, mfGetLogger(), "The entity's id provided isn't valid anymore!");

    if(entity->ownerScene != scene) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The entity provided doesn't belong to this scene!\n");
        return mfnull;
    }

    if(!mfEntityHasTransformComponent(entity))
        return mfnull;

    MFComponentGroup* grp = &mfArrayGet(scene->compGrpTable, MFComponentGroup, entity->compGrpId);
    MFTransformComponent* t = &mfArrayGet(scene->transformCompPool, MFTransformComponent, grp->transformIdx);

    return t;
}

void mfSceneSerialize(MFScene* scene, const char* fileName) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(fileName == mfnull, mfGetLogger(), "The file name shouldn't be null!");

    u64 size = sizeof(u32); // For the scene signature
    size += sizeof(u64) * 4; // For the sizes of the 4 mfarrays
    size += sizeof(MFEntity) * scene->entities.len;
    size += sizeof(MFMeshComponent) * scene->meshCompPool.len;
    size += sizeof(MFTransformComponent) * scene->transformCompPool.len;
    size += sizeof(MFComponentGroup) * scene->compGrpTable.len;
    size += sizeof(char) * 1000000; // Overallocating for any extra lengthy strings

    MFSerializer s = {};
    mfSerializerCreate(&s, size);

    mfSerializeU32(&s, MF_SIGNATURE_SCENE_FILE);

    // Entity array 
    {
        mfSerializeU64(&s, scene->entities.len);
        for(u64 i = 0; i < scene->entities.len; i++) {
            MFEntity* e = &mfArrayGet(scene->entities, MFEntity, i);
            mfSerializeU64(&s, e->uuid);
            mfSerializeU32(&s, e->id);
            mfSerializeU32(&s, e->compGrpId);
            mfSerializeU32(&s, e->components);
        }
    }
    // Mesh comp array 
    {
        mfSerializeU64(&s, scene->meshCompPool.len);
        for(u64 i = 0; i < scene->meshCompPool.len; i++) {
            MFMeshComponent* c = &mfArrayGet(scene->meshCompPool, MFMeshComponent, i);
            mfSerializeString(&s, c->path);
            mfSerializeU64(&s, c->perVertSize);
        }
    }
    // Transform comp array 
    {
        mfSerializeU64(&s, scene->transformCompPool.len);
        for(u64 i = 0; i < scene->transformCompPool.len; i++) {
            MFTransformComponent* t = &mfArrayGet(scene->transformCompPool, MFTransformComponent, i);
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
        mfSerializeU64(&s, scene->compGrpTable.len);
        for(u64 i = 0; i < scene->compGrpTable.len; i++) {
            MFComponentGroup* g = &mfArrayGet(scene->compGrpTable, MFComponentGroup, i);
            mfSerializeU64(&s, g->meshIdx);
            mfSerializeU64(&s, g->transformIdx);
        }
    }

    //mfWriteFile(mfGetLogger(), s.bufferSize, fileName, (const char*)s.buffer, "wb");
    {
        FILE* file = fopen(fileName, "wb");
        MF_PANIC_IF(file == mfnull, mfGetLogger(), "Failed to write to file!");

        fwrite(s.buffer, s.bufferSize, 1, file);

        fclose(file);
    }

    mfSerializerDestroy(&s);
}

b8 mfSceneDeserialize(MFScene* scene, const char* fileName, MFModelVertexBuilder vertexBuilder) {
    MF_PANIC_IF(scene == mfnull, mfGetLogger(), "The scene handle shouldn't be null!");
    MF_PANIC_IF(fileName == mfnull, mfGetLogger(), "The file name shouldn't be null!");

    FILE* file = fopen(fileName, "rb");
    if(file == mfnull)
        return false;
    
    u64 fileSize = 0;
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, SEEK_SET, 0);

    u8* content = MF_ALLOCMEM(u8, fileSize * sizeof(u8));
    fread(content, fileSize, 1, file);

    fclose(file);

    MFSerializer s = {
        .offset = sizeof(u32) * 2,
        .buffer = content,
        .bufferSize = fileSize
    };

    u32 sign = mfDeserializeU32(&s);
    if(sign != MF_SIGNATURE_SCENE_FILE) {
        MF_FREEMEM(content);
        return false;
    }

    mfArrayDestroy(&scene->entities, mfGetLogger());
    mfArrayDestroy(&scene->compGrpTable, mfGetLogger());
    mfArrayDestroy(&scene->meshCompPool, mfGetLogger());
    mfArrayDestroy(&scene->transformCompPool, mfGetLogger());

    u64 eLen = mfDeserializeU64(&s);
    scene->entities = mfArrayCreate(mfGetLogger(), eLen, sizeof(MFEntity));
    for(u64 i = 0; i < eLen; i++) {
        MFEntity e;
        e.ownerScene = scene;
        e.uuid = mfDeserializeU64(&s);
        e.id = mfDeserializeU32(&s);
        e.compGrpId = mfDeserializeU32(&s);
        e.components = mfDeserializeU32(&s);
        mfArrayAddElement(scene->entities, MFEntity, mfGetLogger(), e);
    }

    u64 mLen = mfDeserializeU64(&s);
    scene->meshCompPool = mfArrayCreate(mfGetLogger(), mLen, sizeof(MFMeshComponent));

    for(u64 i = 0; i < mLen; i++) {
        MFMeshComponent c;
        c.path = mfDeserializeString(&s);
        c.perVertSize = mfDeserializeU64(&s);
        c.vertBuilder = vertexBuilder;
        mfModelLoadAndCreate(&c.model, c.path, scene->renderer, c.perVertSize, c.vertBuilder);
        mfArrayAddElement(scene->meshCompPool, MFMeshComponent, mfGetLogger(), c);
    }

    u64 tLen = mfDeserializeU64(&s);
    scene->transformCompPool = mfArrayCreate(mfGetLogger(), tLen, sizeof(MFTransformComponent));
    for(u64 i = 0; i < tLen; i++) {
        MFTransformComponent t;
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
        MFComponentGroup g;
        g.meshIdx = mfDeserializeU64(&s);
        g.transformIdx = mfDeserializeU64(&s);
        mfArrayAddElement(scene->compGrpTable, MFComponentGroup, mfGetLogger(), g);
    }

    mfSerializerDestroy(&s);

    return true;
}
