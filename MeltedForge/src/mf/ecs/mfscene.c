#include "mfscene.h"

void mfSceneCreate(MFScene* scene, MFCamera camera, MFRenderer* renderer) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MF_SETMEM(scene, 0 , sizeof(MFScene));

    scene->camera = camera;
    scene->entities = mfArrayCreate(mfGetLogger(), 2, sizeof(MFEntity));
    scene->renderer = renderer;
}

void mfSceneDestroy(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle provided shouldn't be null!");
    
    MFEntity* list = (MFEntity*)scene->entities.data;    
    for(u32 i = 0; i < scene->entities.len; i++) {
        mfEntityDestroy(&list[i]);
    }

    MF_SETMEM(scene, 0, sizeof(MFScene));
}

//! IMPLEMENT THIS
void mfSceneRender(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle provided shouldn't be null!");
    

}

//! IMPLEMENT THIS
void mfSceneUpdate(MFScene* scene) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle provided shouldn't be null!");


}

void mfSceneAddEntity(MFScene* scene, MFEntity entity) {
    MF_ASSERT(scene == mfnull, mfGetLogger(), "The scene handle provided shouldn't be null!");

    if(scene->entities.len == scene->entities.capacity) {
        mfArrayResize(&scene->entities, scene->entities.capacity * 2, mfGetLogger());
    }

    MFEntity* list = (MFEntity*)scene->entities.data;
    list[scene->entities.len++] = entity;
}