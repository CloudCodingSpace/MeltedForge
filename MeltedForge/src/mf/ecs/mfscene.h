#pragma once

#include "core/mfutils.h"
#include "mfentity.h"
#include "mfcomponents.h"

#include "renderer/mfrenderer.h"
#include "renderer/mfcamera.h"

typedef struct MFScene_s {
    MFArray entities;
    MFArray meshCompPool;
    MFArray transformCompPool;
    MFArray compGrpTable;

    MFCamera camera;
    MFRenderer* renderer;
} MFScene;

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer);
void mfSceneDestroy(MFScene* scene);

void mfSceneRender(MFScene* scene, void (*entityDraw)(MFEntity* e, MFScene* scene, void* state), void* state);
void mfSceneUpdate(MFScene* scene);

const MFEntity* mfSceneCreateEntity(MFScene* scene);
void mfSceneDeleteEntity(MFScene* scene, MFEntity* e);
void mfSceneEntityAddMeshComponent(MFScene* scene, u32 id, MFMeshComponent comp);
void mfSceneEntityAddTransformComponent(MFScene* scene, u32 id, MFTransformComponent comp);

MFMeshComponent* mfSceneEntityGetMeshComponent(MFScene* scene, u32 id);
MFTransformComponent* mfSceneEntityGetTransformComponent(MFScene* scene, u32 id);

void mfSceneSerialize(MFScene* scene, const char* fileName);
