#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mfscene.h"

void mfSceneSerialize(MFScene* scene, const char* fileName);
bool mfSceneDeserialize(MFScene* scene, const char* fileName);

#ifdef __cplusplus
}
#endif