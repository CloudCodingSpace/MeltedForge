#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"
#include "mfgpu_res.h"
#include "mfpipeline.h"
#include "mfutil_types.h"

#include <cimgui.h>

typedef enum MFRenderGraphAttachmentType_e {
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_COLOR_ATTACHMENT = 1 << 0,
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT = 1 << 1,
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_COUNT = 2,
} MFRenderGraphAttachmentType;

typedef struct MFRenderGraphAttachmentDesc_s {
    MFRenderGraphAttachmentType type;
    MFFormat format;
    MFVec3 clearColor;
} MFRenderGraphAttachmentDesc;

typedef struct MFRenderGraphPassDesc_s {
    const char* name;
    u32* depthStencilAttachment;
    u32* outputColorAttachments;
    u32 outputColorAttachmentCount;

    void* userData;
    void (*passDrawCallback)(void* userData);
} MFRenderGraphPassDesc;

typedef struct MFRenderGraphConfig_s {
    u32 attachmentCount;
    u32 passCount;
    u32 width, height;
    MFRenderGraphAttachmentDesc* attachments;
    MFRenderGraphPassDesc* passes;
    void* resizeCallbackUserState;
    void (*resizeCallback)(void* pUserState);
} MFRenderGraphConfig;

typedef struct MFRenderGraph_s MFRenderGraph;

MFRenderGraph* mfRenderGraphCreate(MFRenderer* renderer, MFRenderGraphConfig config);
void mfRenderGraphDestroy(MFRenderGraph** renderGraph);

void mfRenderGraphInvoke(MFRenderGraph* renderGraph, bool waitOnCpu);
void mfRenderGraphResize(MFRenderGraph* renderGraph, u32 width, u32 height);

MFResourceSetLayout* mfRenderGraphGetAttachmentsSetLayout(MFRenderGraph* renderGraph);
void mfRenderGraphBindAttachmentsSet(MFRenderGraph* renderGraph, u64 setIndex, MFPipeline* pipeline);

const MFRenderGraphAttachmentDesc* mfRenderGraphGetAttachment(MFRenderGraph* renderGraph, u32 attachmentIdx);
ImTextureID mfRenderGraphGetAttachmentImTextureID(MFRenderGraph* renderGraph, u32 attachmentIdx);

const MFRenderGraphPassDesc* mfRenderGraphGetPass(MFRenderGraph* renderGraph, u32 passIdx);
const MFRenderGraphConfig* mfRenderGraphGetConfig(MFRenderGraph* renderGraph);

#ifdef __cplusplus
}
#endif