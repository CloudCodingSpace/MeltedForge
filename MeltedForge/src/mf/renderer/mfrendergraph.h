#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"
#include "core/mfcore.h"

#include "mfrenderer.h"
#include "mfutil_types.h"

typedef enum MFRenderGraphAttachmentType_e {
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_COLOR_ATTACHMENT = 1 << 0,
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_INPUT_ATTACHMENT = 1 << 1,
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT = 1 << 2,
    MF_RENDER_GRAPH_ATTACHMENT_TYPE_COUNT = 3,
} MFRenderGraphAttachmentType;

typedef struct MFRenderGraphAttachmentDesc_s {
    MFRenderGraphAttachmentType type;
    MFFormat format;
    MFVec4 clearColor;
} MFRenderGraphAttachmentDesc;

typedef struct MFRenderGraphPassDesc_s {
    const char* name;
    u32* inputAttachments;
    u32* depthStencilAttachment;
    u32* outputColorAttachments;
    u32 inputAttachmentCount;
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
} MFRenderGraphConfig;

typedef struct MFRenderGraph_s MFRenderGraph;

MFRenderGraph* mfRenderGraphCreate(MFRenderer* renderer, MFRenderGraphConfig config);
void mfRenderGraphDestroy(MFRenderGraph** renderGraph);

void mfRenderGraphInvoke(MFRenderGraph* renderGraph);
const MFRenderGraphConfig* mfRenderGraphGetConfig(MFRenderGraph* renderGraph);

#ifdef __cplusplus
}
#endif