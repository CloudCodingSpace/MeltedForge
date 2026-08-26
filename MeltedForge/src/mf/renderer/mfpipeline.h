#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MFPipeline_s MFPipeline;

#include "core/mfutils.h"

#include "mfutil_types.h"
#include "mfgpu_res.h"

#include "mfrenderer.h"

struct MFRenderGraph_s;

typedef enum MFPipelineType_e {
    MF_PIPELINE_TYPE_GRAPHICS,
    MF_PIPELINE_TYPE_COMPUTE,
    MF_PIPELINE_TYPE_COUNT
} MFPipelineType;

typedef struct MFGraphicsPipelineConfig_s {
    u32 bindingsCount, attributesCount, renderGraphPassIdx;
    MFCompareOp depthCompareOp;
    bool hasDepth, transparent;
    const char* vertPath;
    const char* fragPath;
    struct MFRenderGraph_s* renderGraph;
    MFVec2 extent;
    MFCullModeFlags cullMode;
    MFVertexInputBindingDescription* bindings;
    MFVertexInputAttributeDescription* attributes;
} MFGraphicsPipelineConfig;

typedef struct MFComputePipelineConfig_s {
    const char* filePath;
} MFComputePipelineConfig;

typedef struct MFPipelineConfig_s {
    u32 resourceLayoutCount, pushConstRangeCount;
    MFPushConstantRange* pushConstRanges;
    MFResourceSetLayout** resourceLayouts;
    
    MFPipelineType type;
    MFGraphicsPipelineConfig graphicsConfig;
    MFComputePipelineConfig computeConfig;
} MFPipelineConfig;

MFPipeline* mfPipelineCreate(MFRenderer* renderer, MFPipelineConfig config);
void mfPipelineDestroy(MFPipeline** pipeline);

void mfPipelinePrepareComputeDispatch(MFPipeline* pipeline);
void mfPipelineComputeDispatch(MFPipeline* pipeline, u32 workgroupSizeX, u32 workgroupSizeY);

void mfPipelinePushConstant(MFPipeline* pipeline, MFShaderStage shaderStage, u32 offset, u32 size, void* data);
void mfPipelineBind(MFPipeline* pipeline, MFViewport vp, MFRect2D scissor);

void* mfPipelineGetLayoutBackend(MFPipeline* pipeline);
void* mfPipelineGetBackend(MFPipeline* pipeline);
size_t mfPipelineGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif
