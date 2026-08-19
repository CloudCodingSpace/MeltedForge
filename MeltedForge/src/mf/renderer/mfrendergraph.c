#ifdef __cplusplus
extern "C" {
#endif

#include "mfrendergraph.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/image.h"
#include "vk/fb.h"

/* 
 * TODO: Plan for a pass object, attachment object, and figure out how to return each attachment's imgui set if required 
 *       Also handle how to return each attachment's resource set or its image handle in case the client needs to get 
 *       its pixel data etc
*/

struct MFRenderGraph_s {
    MFRenderGraphConfig config;
    bool init, began;
    MFRenderer* renderer;
    VulkanImage* attachments;
    VulkanFramebuffer fb;
    VulkanRenderPass pass;
};

MFRenderGraph* mfRenderGraphCreate(MFRenderer* renderer, MFRenderGraphConfig config) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(config.attachmentCount == 0 || !config.attachments, mfGetLogger(), "The no. of attachments in a rendergraph musn't be zero and the attachment array pointer musn't be null!");
    MF_PANIC_IF(config.width == 0 || config.height == 0, mfGetLogger(), "The width & height of the rendergraph musn't be null!");
    MF_PANIC_IF(config.passes == 0 || !config.passes, mfGetLogger(), "The no. of passes in a rendergraph musn't be zero and the passes array pointer musn't be null!");

    // Verification of data
    {
        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &config.passes[i];

            if(i == config.passCount - 1)
                MF_PANIC_IF(pass->outputColorAttachmentCount == 0 || !pass->outputColorAttachments, mfGetLogger(), "Each rendergraph's pass must have atleast one output attachment & so the output attachment's array mustn't be null!");

            for(u32 j = 0; j < pass->inputAttachmentCount; j++) {
                u32 idx = pass->inputAttachments[i];

                MF_PANIC_IF(idx >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
            }

            for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                u32 idx = pass->outputColorAttachments[i];

                MF_PANIC_IF(idx >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
            }

            if(pass->depthStencilAttachment != mfnull)
                MF_PANIC_IF(pass->depthStencilAttachment[0] >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
        }
    }

    MFRenderGraph* renderGraph = MF_ALLOCMEM(MFRenderGraph, sizeof(MFRenderGraph));
    renderGraph->renderer = renderer;

    // Copying config data into the rendergraph
    {
        renderGraph->config.width = config.width;
        renderGraph->config.height = config.height;
        renderGraph->config.passCount = config.passCount;
        renderGraph->config.attachmentCount = config.attachmentCount;

        renderGraph->config.attachments = MF_ALLOCMEM(MFRenderGraphAttachmentDesc, sizeof(MFRenderGraphAttachmentDesc) * config.attachmentCount);
        memcpy(renderGraph->config.attachments, config.attachments, sizeof(MFRenderGraphAttachmentDesc) * config.attachmentCount);
        
        renderGraph->config.passes = MF_ALLOCMEM(MFRenderGraphPassDesc, sizeof(MFRenderGraphPassDesc) * config.passCount);
        memcpy(renderGraph->config.passes, config.passes, sizeof(MFRenderGraphPassDesc) * config.passCount);
    }

    // TODO: Create the attachments, framebuffer, renderpass, etc
    // Attachments
    {

    }
    // RenderPass
    {

    }
    // Framebuffer
    {

    }

    renderGraph->init = true;
    return renderGraph;
}

void mfRenderGraphDestroy(MFRenderGraph** _renderGraph) {
    MF_PANIC_IF(_renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    
    MFRenderGraph* renderGraph = _renderGraph[0];
    
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");

    MF_FREEMEM(renderGraph->config.attachments);
    MF_FREEMEM(renderGraph->config.passes);

    MF_SETMEM(renderGraph, 0, sizeof(MFRenderGraph));
    MF_FREEMEM(renderGraph);
    MF_SETMEM(_renderGraph, 0, sizeof(MFRenderGraph*));
}

void mfRenderGraphInvoke(MFRenderGraph* renderGraph) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    
    // TODO: Fill this thing out
}


void mfRenderGraphResize(MFRenderGraph* renderGraph, u32 width, u32 height) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(width == 0 || height == 0, mfGetLogger(), "The width & height of the rendergraph extent musn't be null!");
    
    // TODO: Fill this thing out
}

const MFRenderGraphAttachmentDesc* mfRenderGraphGetAttachment(MFRenderGraph* renderGraph, u32 attachmentIdx) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(attachmentIdx >= renderGraph->config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
    
    return &renderGraph->config.attachments[attachmentIdx];
}

const MFRenderGraphPassDesc* mfRenderGraphGetPass(MFRenderGraph* renderGraph, u32 passIdx) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(passIdx >= renderGraph->config.passCount, mfGetLogger(), "The pass's index reference is out of bounds of the total no. of passes provided to the rendergraph!");
    
    return &renderGraph->config.passes[passIdx];
}

const MFRenderGraphConfig* mfRenderGraphGetConfig(MFRenderGraph* renderGraph) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");

    return &renderGraph->config;
}

#ifdef __cplusplus
}
#endif