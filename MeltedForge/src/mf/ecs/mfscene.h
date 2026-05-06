#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfarray.h"

#include "mfentity.h"
#include "mfcomponents.h"

#include "renderer/mfrenderer.h"
#include "renderer/mfpipeline.h"

#include "objects/mfcamera.h"
#include "window/mfwindow.h"

typedef struct MFSceneRenderConfig_s {
    MFPipeline* entityPipeline;
    MFViewport viewport;
    MFRect2D scissor;
    void* state;

    MFMat4 (*computeModelMatrix)(const MFTransformComponent* component);
    void (*perMeshDrawCallback)(void* state, MFMat4 transform, const MFMeshComponent* component, u64 meshIdx, MFPipeline* pipeline);
    void (*pipelineBindCallback)(void* state, MFPipeline* pipeline);
} MFSceneRenderConfig;

typedef struct MFScene_s {
    MFArray entities;
    MFArray meshCompPool;
    MFArray transformCompPool;
    MFArray compGrpTable;

    MFModelVertexBuilder vertBuilder;
    MFCamera camera;
    MFRenderer* renderer;
    bool init;
} MFScene;

void mfSceneCreate(MFScene* scene, MFCamera camera, MFModelVertexBuilder vertBuilder, MFRenderer* renderer);
void mfSceneDestroy(MFScene* scene);

void mfSceneRender(MFScene* scene, MFSceneRenderConfig* config);
void mfSceneUpdate(MFScene* scene);

u64 mfSceneCreateEntity(MFScene* scene);
void mfSceneDeleteEntity(MFScene* scene, u64* id);

void mfSceneEntityAddMeshComponent(MFScene* scene, u64* id, MFMeshComponent comp);
void mfSceneEntityAddTransformComponent(MFScene* scene, u64* id, MFTransformComponent comp);

void mfSceneEntityRemoveMeshComponent(MFScene* scene, u64* id);
void mfSceneEntityRemoveTransformComponent(MFScene* scene, u64* id);

MFMeshComponent* mfSceneEntityGetMeshComponent(MFScene* scene, u64* id);
MFTransformComponent* mfSceneEntityGetTransformComponent(MFScene* scene, u64* id);

void mfSceneGetValidEntities(MFScene* scene, u64* validEntityCount, MFEntity* entities);

#ifdef __cplusplus
}
#endif