#pragma once

#include "core/mfutils.h"
#include "renderer/mfmodel.h"

MFArray mfMaterialSystemGetModelMatImages(MFModel* model, void* renderer);
void mfMaterialSystemDeleteModelMatImages(MFArray* array);