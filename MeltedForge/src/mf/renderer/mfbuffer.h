#pragma once

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"

typedef enum MFGpuBufferTypes_s {
    MF_GPU_BUFFER_TYPE_NONE,
    MF_GPU_BUFFER_TYPE_VERTEX,
    MF_GPU_BUFFER_TYPE_INDEX
} MFGpuBufferTypes;

typedef struct MFGpuBufferConfig_s {
    u64 size;
    void* data;
    MFGpuBufferTypes type;
} MFGpuBufferConfig;

typedef struct MFGpuBuffer_s MFGpuBuffer;

void mfGpuBufferAllocate(MFGpuBuffer* buffer, MFGpuBufferConfig config, MFRenderer* renderer);
void mfGpuBufferFree(MFGpuBuffer* buffer);

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data);
void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data);

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer);

size_t mfGpuBufferGetSizeInBytes();