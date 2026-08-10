#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfcore.h"
#include "core/mfarray.h"

#include "mfutil_types.h"

struct MFGpuImage_s;
struct MFGpuBuffer_s;

typedef struct MFRenderer_s MFRenderer;
typedef struct MFResourceSetLayout_s MFResourceSetLayout;
typedef struct MFResourceSet_s MFResourceSet;
struct MFPipeline_s;

MFResourceSetLayout* mfResourceSetLayoutCreate(u64 bindingLen, MFResourceSetBindings* bindings, u64 maxSets, MFRenderer* renderer);
void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout);

MFResourceSet* mfResourceSetCreate(MFResourceSetLayout* layout, MFRenderer* renderer);
void mfResourceSetDestroy(MFResourceSet* set);

void mfResourceSetsBind(u32 firstSetIndex, u64 setCount, MFResourceSet** sets, struct MFPipeline_s* pipeline);
void mfResourceSetUpdate(MFResourceSet* set, u32 imageCount, struct MFGpuImage_s** images, u32 bufferCount, struct MFGpuBuffer_s** buffers);

void* mfResourceSetLayoutGetBackend(MFResourceSetLayout* layout);
void** mfResourceSetGetBackend(MFResourceSet* set);

size_t mfResourceSetLayoutGetSizeInBytes(void);
size_t mfResourceSetGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif