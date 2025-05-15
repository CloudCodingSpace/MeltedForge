#pragma once

typedef struct MFPipeline_s MFPipeline;

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"

#include "mfrenderer.h"

typedef struct MFPipelineInfo_s {
    MFVec2 extent;
    u32 bindingDescsCount, attribDescsCount;
    MFVertexInputBindingDescription* bindingDescs;
    MFVertexInputAttributeDescription* attribDescs;
    b8 hasDepth;
    const char* vertPath;
    const char* fragPath;
} MFPipelineInfo;

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineInfo* info);
void mfPipelineDestroy(MFPipeline* pipeline);

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor);

size_t mfPipelineGetSizeInBytes();