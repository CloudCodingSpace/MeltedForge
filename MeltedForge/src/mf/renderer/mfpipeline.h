#pragma once

typedef struct MFPipeline_s MFPipeline;

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfgpuimage.h"
#include "mfgpubuffer.h"
#include "mfgpu_res.h"

#include "mfrenderer.h"

typedef struct MFPipelineConfig_s {
    MFVec2 extent;
    u32 bindingsCount, attributesCount, resourceLayoutCount;
    MFVertexInputBindingDescription* bindings;
    MFVertexInputAttributeDescription* attributes;
    MFResourceSetLayout** resourceLayouts;
    MFCompareOp depthCompareOp;
    b8 hasDepth, transparent;
    const char* vertPath;
    const char* fragPath;
    void* renderpass;
} MFPipelineConfig;

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineConfig* info);
void mfPipelineDestroy(MFPipeline* pipeline);

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor);

void* mfPipelineGetLayoutBackend(MFPipeline* pipeline);
void* mfPipelineGetBackend(MFPipeline* pipeline);
size_t mfPipelineGetSizeInBytes(void);