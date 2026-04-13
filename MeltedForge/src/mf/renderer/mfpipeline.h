#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MFPipeline_s MFPipeline;

#include "core/mfcore.h"
#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfrender_target.h"
#include "mfgpuimage.h"
#include "mfgpubuffer.h"
#include "mfgpu_res.h"

#include "mfrenderer.h"

typedef struct MFPipelineConfig_s {
    MFVec2 extent;
    u32 bindingsCount, attributesCount, resourceLayoutCount, pushConstRangeCount;
    MFVertexInputBindingDescription* bindings;
    MFVertexInputAttributeDescription* attributes;
    MFPushConstantRange* pushConstRanges;
    MFResourceSetLayout** resourceLayouts;
    MFCompareOp depthCompareOp;
    b8 hasDepth, transparent;
    const char* vertPath;
    const char* fragPath;
    MFRenderTarget* renderTarget;
} MFPipelineConfig;

void mfPipelineInit(MFPipeline* pipeline, MFRenderer* renderer, MFPipelineConfig* info);
void mfPipelineDestroy(MFPipeline* pipeline);

void mfPipelinePushConstant(MFPipeline* pipeline, MFShaderStage shaderStage, u32 offset, u32 size, void* data);
void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor);

void* mfPipelineGetLayoutBackend(MFPipeline* pipeline);
void* mfPipelineGetBackend(MFPipeline* pipeline);
size_t mfPipelineGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif