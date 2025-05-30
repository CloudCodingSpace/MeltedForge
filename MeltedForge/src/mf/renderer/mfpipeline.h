#pragma once

typedef struct MFPipeline_s MFPipeline;

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfimage.h"

#include "mfrenderer.h"

typedef struct MFPipelineConfig_s {
    MFVec2 extent;
    u32 bindingDescsCount, attribDescsCount, imgCount;
    MFVertexInputBindingDescription* bindingDescs;
    MFVertexInputAttributeDescription* attribDescs;
    MFGpuImage** images;
    b8 hasDepth;
    const char* vertPath;
    const char* fragPath;
} MFPipelineConfig;

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineConfig* info);
void mfPipelineDestroy(MFPipeline* pipeline);

void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor);

size_t mfPipelineGetSizeInBytes();