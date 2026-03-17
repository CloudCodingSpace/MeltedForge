#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfrenderer.h"

typedef struct MFResourceSetLayout_s MFResourceSetLayout;
typedef struct MFResourceSet_s MFResourceSet;
struct MFPipeline_s;

void mfResourceSetLayoutCreate(MFResourceSetLayout* layout, u64 resDescLen, MFResourceDesc* resDescs, MFRenderer* renderer);
void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout);

void mfResourceSetCreate(MFResourceSet* set, MFResourceSetLayout* layout, MFRenderer* renderer);
void mfResourceSetDestroy(MFResourceSet* set);

void mfResourceSetBind(MFResourceSet* set, struct MFPipeline_s* pipeline);
void mfResourceSetUpdate(MFResourceSet* set, MFArray* images, MFArray* buffers);

void* mfGetResourceSetLayoutBackend(MFResourceSetLayout* layout);

size_t mfGetResourceSetLayoutSizeInBytes(void);
size_t mfGetResourceSetSizeInBytes(void);