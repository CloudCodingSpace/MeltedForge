#pragma once

#include "core/mfutils.h"
#include "mfentity.h"

#include "renderer/mfrenderer.h"
#include "renderer/mfcamera.h"
#include "window/mfwindow.h"

typedef struct MFScene_s {
    MFArray entities;
    MFCamera camera;

    MFRenderer* renderer;
} MFScene;

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer);
void mfSceneDestroy(MFScene* scene);

void mfSceneRender(MFScene* scene);
void mfSceneUpdate(MFScene* scene);

void mfSceneAddEntity(MFScene* scene, MFEntity entity);