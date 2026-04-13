#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfrenderer.h"

typedef struct MFResourceSetLayout_s MFResourceSetLayout;
typedef struct MFResourceSet_s MFResourceSet;
struct MFPipeline_s;

void mfResourceSetLayoutCreate(MFResourceSetLayout* layout, u64 resDescLen, MFResourceDescription* resDescs, u64 maxSets, MFRenderer* renderer);
void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout);

void mfResourceSetCreate(MFResourceSet* set, MFResourceSetLayout* layout, MFRenderer* renderer);
void mfResourceSetDestroy(MFResourceSet* set);

void mfResourceSetBind(MFResourceSet* set, struct MFPipeline_s* pipeline);
void mfResourceSetUpdate(MFResourceSet* set, MFArray* images, MFArray* buffers);

void* mfResourceSetLayoutGetBackend(MFResourceSetLayout* layout);

size_t mfResourceSetLayoutGetSizeInBytes(void);
size_t mfResourceSetGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif