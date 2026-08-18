#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"
#include "mfutil_types.h"

typedef enum MFGpuBufferTypes_s {
    MF_GPU_BUFFER_TYPE_NONE,
    MF_GPU_BUFFER_TYPE_VERTEX,
    MF_GPU_BUFFER_TYPE_INDEX,
    MF_GPU_BUFFER_TYPE_UBO,
    MF_GPU_BUFFER_TYPE_SSBO
} MFGpuBufferTypes;

typedef struct MFGpuBufferConfig_s {
    u64 size;
    MFShaderStage stage;
    void* data;
    MFGpuBufferTypes type;
    bool frequentUpdates, frameSynced;
} MFGpuBufferConfig;

typedef struct MFGpuBuffer_s MFGpuBuffer;

MFGpuBuffer* mfGpuBufferAllocate(MFGpuBufferConfig config, MFRenderer* renderer);
void mfGpuBufferFree(MFGpuBuffer** buffer);

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data);
void* mfGpuBufferGetData(MFGpuBuffer* buffer);
void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data);

void mfGpuBufferBind(MFGpuBuffer* buffer);

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer);
MFResourceDescription mfGpuBufferGetDescription(MFGpuBuffer* buffer);
void* mfGpuBufferGetBackend(MFGpuBuffer* buffer);

size_t mfGpuBufferGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif