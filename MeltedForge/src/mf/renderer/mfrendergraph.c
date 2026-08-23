#ifdef __cplusplus
extern "C" {
#endif

#include "mfrendergraph.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/image.h"
#include "vk/fb.h"
#include "vk/command_buffer.h"

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
    VulkanFramebuffer fbs[FRAMES_IN_FLIGHT];
    VkRenderPass pass;

    VkSemaphore* renderFinishedSemas;
    VkFence fences[FRAMES_IN_FLIGHT];
    VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
};

MFRenderGraph* mfRenderGraphCreate(MFRenderer* renderer, MFRenderGraphConfig config) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(config.attachmentCount == 0 || !config.attachments, mfGetLogger(), "The no. of attachments in a rendergraph musn't be zero and the attachment array pointer musn't be null!");
    MF_PANIC_IF(config.width == 0 || config.height == 0, mfGetLogger(), "The width & height of the rendergraph musn't be null!");
    MF_PANIC_IF(config.passes == 0 || !config.passes, mfGetLogger(), "The no. of passes in a rendergraph musn't be zero and the passes array pointer musn't be null!");

    // TODO: Later also verify for memory hazards for passes!
    // Verification of data
    {
        for(u32 i = 0; i < config.attachmentCount; i++) {
            MFRenderGraphAttachmentDesc* attachment = &config.attachments[i];

            MF_PANIC_IF(mfFlagContainsBits(attachment->type, MF_RENDER_GRAPH_ATTACHMENT_TYPE_COLOR_ATTACHMENT) && mfFlagContainsBits(attachment->type, MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT), mfGetLogger(),
                "A attachment of a rendergraph can't be both used as a color attachment and a depth stencil attachment at the same time!");
        }

        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &config.passes[i];

            if(i == config.passCount - 1)
                MF_PANIC_IF(pass->outputColorAttachmentCount == 0 || !pass->outputColorAttachments, mfGetLogger(), "Each rendergraph's pass must have atleast one output attachment & so the output attachment's array mustn't be null!");

            for(u32 j = 0; j < pass->inputAttachmentCount; j++) {
                u32 idx = pass->inputAttachments[j];

                MF_PANIC_IF(idx >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
                // Just in case if not added, adding the input attachment flag
                config.attachments[idx].type |= MF_RENDER_GRAPH_ATTACHMENT_TYPE_INPUT_ATTACHMENT;
            }

            for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                u32 idx = pass->outputColorAttachments[j];

                MF_PANIC_IF(idx >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
            }

            if(pass->depthStencilAttachment != mfnull) {
                MF_PANIC_IF(pass->depthStencilAttachment[0] >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
                MF_PANIC_IF(!mfFlagContainsBits(config.attachments[pass->depthStencilAttachment[0]].type, MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT), mfGetLogger(),
                        "The given depth stencil attachment of a pass of a rendergraph must be a index reference to a attachment which is actually a depth stencil attachment instead of some other type of attachment!");
            }
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

        // NOTE: I hate allocating these arrays but I still need to for book-keeping T-T
        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &renderGraph->config.passes[i];
            pass->name = mfStringDuplicate(config.passes[i].name);

            if(pass->outputColorAttachmentCount > 0) {
                u32* outputAttachments = MF_ALLOCMEM(u32, sizeof(u32) * pass->outputColorAttachmentCount);
                
                for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                    outputAttachments[j] = pass->outputColorAttachments[j];
                }
                pass->outputColorAttachments = outputAttachments;
            }

            if(pass->inputAttachmentCount > 0) {
                u32* inputAttachments = MF_ALLOCMEM(u32, sizeof(u32) * pass->inputAttachmentCount);
                
                for(u32 j = 0; j < pass->inputAttachmentCount; j++) {
                    inputAttachments[j] = pass->inputAttachments[j];
                }
                pass->inputAttachments = inputAttachments;
            }

            if(pass->depthStencilAttachment != mfnull) {
                u32 id = pass->depthStencilAttachment[0];
                pass->depthStencilAttachment = MF_ALLOCMEM(u32, sizeof(u32));
                pass->depthStencilAttachment[0] = id;
            }
        }
    }

    // TODO: Add MSAA support for rendergraph attachments!
    // TODO: Create the framebuffers, renderpass, etc

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    // Attachments
    {
        renderGraph->attachments = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * config.attachmentCount);
        
        for(u32 i = 0; i < config.attachmentCount; i++) {
            MFRenderGraphAttachmentDesc* desc = &config.attachments[i];
            bool isDepth = mfFlagContainsBits(desc->type ,MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT);

            VulkanImageInfo info = {
                .arrayLayers = 1,
                .aspectFlags = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                .ctx = ctx,
                .format = (VkFormat)(u32)desc->format,
                .gpuResource = true,
                .width = config.width,
                .height = config.height,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .type = VK_IMAGE_TYPE_2D,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT
            };

            if(mfFlagContainsBits(desc->type, MF_RENDER_GRAPH_ATTACHMENT_TYPE_COLOR_ATTACHMENT))
                info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if(mfFlagContainsBits(desc->type, MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT))
                info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            if(mfFlagContainsBits(desc->type, MF_RENDER_GRAPH_ATTACHMENT_TYPE_INPUT_ATTACHMENT))
                info.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            
            VulkanImageCreate(&renderGraph->attachments[i], info);
        }
    }
    // RenderPass
    {
        VkAttachmentDescription* attachments = MF_ALLOCMEM(VkAttachmentDescription, sizeof(VkAttachmentDescription) * config.attachmentCount);
        VkSubpassDescription* passes = MF_ALLOCMEM(VkSubpassDescription, sizeof(VkSubpassDescription) * config.passCount);
        VkSubpassDependency* dependencies = MF_ALLOCMEM(VkSubpassDependency, sizeof(VkSubpassDependency) * config.passCount);

        u32 attachmentRefCount = 0;
        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &config.passes[i];

            if(pass->inputAttachmentCount > 0)
                attachmentRefCount++;
            if(pass->outputColorAttachmentCount > 0)
                attachmentRefCount++;
            if(pass->depthStencilAttachment != mfnull)
                attachmentRefCount++;
        }

        // NOTE: I genuinely fucking hate this ** array thing right here, moreover since it is heap allocated but atm due to my lack of expertise, I can't find a better solution T-T
        VkAttachmentReference** attachmentRefs = MF_ALLOCMEM(VkAttachmentReference*, sizeof(VkAttachmentReference*) * attachmentRefCount);

        for(u32 i = 0; i < config.attachmentCount; i++) {
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachments[i].format = (VkFormat)(u32)config.attachments[i].format;
            attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        // TODO: Translate the passes
        u64 attachmentRefIdx = 0;
        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* passDesc = &config.passes[i];
            
            passes[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: Make it configurable if required

            if(passDesc->inputAttachmentCount > 0) {
                VkAttachmentReference* inputRefs = MF_ALLOCMEM(VkAttachmentReference, sizeof(VkAttachmentReference) * passDesc->inputAttachmentCount);
    
                for(u32 j = 0; j < passDesc->inputAttachmentCount; j++) {
                    inputRefs[j].attachment = passDesc->inputAttachments[j];
                    inputRefs[j].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
    
                passes[i].inputAttachmentCount = passDesc->inputAttachmentCount;
                passes[i].pInputAttachments = inputRefs;
                attachmentRefs[attachmentRefIdx++] = inputRefs;

            }

            if(passDesc->depthStencilAttachment != mfnull) {
                VkAttachmentReference* ref = MF_ALLOCMEM(VkAttachmentReference, sizeof(VkAttachmentReference));

                ref->attachment = passDesc->depthStencilAttachment[0];
                ref->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                passes[i].pDepthStencilAttachment = ref;
                attachmentRefs[attachmentRefIdx++] = ref;
            }
            
            if(passDesc->outputColorAttachmentCount > 0) {
                VkAttachmentReference* colorRefs = MF_ALLOCMEM(VkAttachmentReference, sizeof(VkAttachmentReference) * passDesc->outputColorAttachmentCount);

                for(u32 j = 0; j < passDesc->outputColorAttachmentCount; j++) {
                    colorRefs[j].attachment = passDesc->outputColorAttachments[j];
                    colorRefs[j].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }

                passes[i].colorAttachmentCount = passDesc->outputColorAttachmentCount;
                passes[i].pColorAttachments = colorRefs;
                attachmentRefs[attachmentRefIdx++] = colorRefs;
            }
        }

        // TODO: Later find out and use the most accurate dependency masks for each inter pass dependency
        VkSubpassDependency dependency = {0};
        {
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        dependencies[0] = dependency;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;

        for(u32 i = 1; i < config.passCount; i++) {
            dependencies[i].srcSubpass = i - 1;
            dependencies[i].dstSubpass = i;
            dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            MFRenderGraphPassDesc* pass = &config.passes[i];
            if((pass->inputAttachmentCount > 0) && (pass->inputAttachments != mfnull)) {
                dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[i].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[i].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            }
        }

        VkRenderPassCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = config.attachmentCount,
            .pAttachments = attachments,
            .dependencyCount = config.passCount,
            .pDependencies = dependencies,
            .subpassCount = config.passCount,
            .pSubpasses = passes
        };

        VK_CHECK(vkCreateRenderPass(ctx->device, &info, ctx->allocator, &renderGraph->pass));

        for(u32 i = 0; i < attachmentRefCount; i++)
            MF_FREEMEM(attachmentRefs[i]);
    
        MF_FREEMEM(attachmentRefs);
        MF_FREEMEM(attachments);
        MF_FREEMEM(dependencies);
        MF_FREEMEM(passes);
    }
    // Framebuffer
    {
        VulkanImage** attachments = MF_ALLOCMEM(VulkanImage*, sizeof(VulkanImage*) * config.attachmentCount);
        for(u32 i = 0; i < config.attachmentCount; i++) {
            attachments[i] = &renderGraph->attachments[i];
        }

        for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VulkanFramebufferCreate(&renderGraph->fbs[i], ctx, renderGraph->pass, config.attachmentCount, attachments, (VkExtent2D){ config.width, config.height });
        }

        MF_FREEMEM(attachments);
    }
    // Sync Objects and command buffer
    {
        renderGraph->renderFinishedSemas = MF_ALLOCMEM(VkSemaphore, sizeof(VkSemaphore) * ctx->swapchainImageCount);

        VkSemaphoreCreateInfo semaInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };


        for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
            VK_CHECK(vkCreateSemaphore(ctx->device, &semaInfo, ctx->allocator, &renderGraph->renderFinishedSemas[i]));
        }

        for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            renderGraph->commandBuffers[i] = VulkanCommandBufferAllocate(ctx, ctx->commandPool, true);
            VK_CHECK(vkCreateFence(ctx->device, &fenceInfo, ctx->allocator, &renderGraph->fences[i]));
        }
        VK_CHECK(vkResetFences(ctx->device, FRAMES_IN_FLIGHT, renderGraph->fences));
    }

    renderGraph->init = true;
    return renderGraph;
}

void mfRenderGraphDestroy(MFRenderGraph** _renderGraph) {
    MF_PANIC_IF(_renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    
    MFRenderGraph* renderGraph = _renderGraph[0];
    
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderGraph->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VulkanFramebufferDestroy(&renderGraph->fbs[i]);
    }

    vkDestroyRenderPass(ctx->device, renderGraph->pass, ctx->allocator);

    for(u32 i = 0; i < renderGraph->config.attachmentCount; i++) {
        VulkanImageDestroy(&renderGraph->attachments[i]);
    }

    for(u32 i = 0; i < renderGraph->config.passCount; i++) {
        MF_FREEMEM(renderGraph->config.passes[i].name);
        MF_FREEMEM(renderGraph->config.passes[i].outputColorAttachments);
        MF_FREEMEM(renderGraph->config.passes[i].depthStencilAttachment);
        MF_FREEMEM(renderGraph->config.passes[i].inputAttachments);
    }

    for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VulkanCommandBufferFree(ctx, renderGraph->commandBuffers[i], ctx->commandPool);
        vkDestroyFence(ctx->device, renderGraph->fences[i], ctx->allocator);
    }

    for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
        vkDestroySemaphore(ctx->device, renderGraph->renderFinishedSemas[i], ctx->allocator);
    }
    
    MF_FREEMEM(renderGraph->renderFinishedSemas);
    MF_FREEMEM(renderGraph->attachments);
    MF_FREEMEM(renderGraph->config.attachments);
    MF_FREEMEM(renderGraph->config.passes);

    MF_SETMEM(renderGraph, 0, sizeof(MFRenderGraph));
    MF_FREEMEM(renderGraph);
    MF_SETMEM(_renderGraph, 0, sizeof(MFRenderGraph*));
}

void mfRenderGraphInvoke(MFRenderGraph* renderGraph, bool waitOnCpu) {
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