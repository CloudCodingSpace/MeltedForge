#pragma once

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"
#include "mfutil_types.h"

struct VulkanBuffer_s;

typedef enum MFGpuBufferTypes_s {
    MF_GPU_BUFFER_TYPE_NONE,
    MF_GPU_BUFFER_TYPE_VERTEX,
    MF_GPU_BUFFER_TYPE_INDEX,
    MF_GPU_BUFFER_TYPE_UBO
} MFGpuBufferTypes;

typedef struct MFGpuBufferConfig_s {
    u64 size, binding; // NOTE: The shader binding must be specified for a UBO
    MFShaderStage stage; // NOTE: The shader stage should be specifies for a UBO
    void* data;
    MFGpuBufferTypes type;
} MFGpuBufferConfig;

typedef struct MFGpuBuffer_s MFGpuBuffer;

void mfGpuBufferAllocate(MFGpuBuffer* buffer, MFGpuBufferConfig config, MFRenderer* renderer);
void mfGpuBufferFree(MFGpuBuffer* buffer);

void mfGpuBufferUploadData(MFGpuBuffer* buffer, void* data);
void mfGpuBufferResize(MFGpuBuffer* buffer, u64 size, void* data);

void mfGpuBufferBind(MFGpuBuffer* buffer);

const MFGpuBufferConfig* mfGpuBufferGetConfig(MFGpuBuffer* buffer);

size_t mfGpuBufferGetSizeInBytes();
MFResourceDesc mfGetGpuBufferDescription(MFGpuBuffer* buffer);
struct VulkanBuffer_s* mfGetGpuBufferBackend(MFGpuBuffer* buffer);