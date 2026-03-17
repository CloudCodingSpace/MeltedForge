#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfrenderer.h"

typedef struct MFResourceSetLayout_s MFResourceSetLayout;
typedef struct MFResourceSet_s MFResourceSet;

void mfResourceSetLayoutCreate(MFResourceSetLayout* layout, u64 resDescLen, MFResourceDesc* resDescs, MFRenderer* renderer);
void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout);

void mfResourceSetCreate(MFResourceSet* set, MFResourceSetLayout* layout, MFRenderer* renderer);
void mfResourceSetDestroy(MFResourceSet* set);

void mfResourceSetUpdate(MFResourceSet* set);

void* mfGetResourceSetLayoutBackend(MFResourceSetLayout* layout);

size_t mfGetResourceSetLayoutSizeInBytes(void);
size_t mfGetResourceSetSizeInBytes(void);