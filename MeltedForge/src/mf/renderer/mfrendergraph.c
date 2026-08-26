#ifdef __cplusplus
extern "C" {
#endif

#include "mfrendergraph.h"

#include "mfgpu_res.h"

#include "vk/backend.h"
#include "vk/ctx.h"
#include "vk/command_buffer.h"
#include "vk/rendergraph.h"
#include "vk/pipeline.h"

#include <cimgui_impl.h>

/* 
 *  TODO: Also handle how to return each attachment's resource set or its image handle in case the client needs to get 
 *        its pixel data etc
*/

// TODO: HAVE MULTIPLE RENDERPASSES INSTEAD OF MULTIPLE SUBPASSES!!!!!!!!!!!!!!!!!

MFRenderGraph* mfRenderGraphCreate(MFRenderer* renderer, MFRenderGraphConfig config) {
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(config.attachmentCount == 0 || !config.attachments, mfGetLogger(), "The no. of attachments in a rendergraph musn't be zero and the attachment array pointer musn't be null!");
    MF_PANIC_IF(config.width == 0 || config.height == 0, mfGetLogger(), "The width & height of the rendergraph musn't be null!");
    MF_PANIC_IF(config.passes == 0 || !config.passes, mfGetLogger(), "The no. of passes in a rendergraph musn't be zero and the passes array pointer musn't be null!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

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

            MF_PANIC_IF(pass->outputColorAttachmentCount == 0 || !pass->outputColorAttachments, mfGetLogger(), "Each rendergraph's pass must have atleast one output attachment & so the output attachment's array mustn't be null!");

            for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                u32 idx = pass->outputColorAttachments[j];

                MF_PANIC_IF(idx >= config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
            }

            if(!backend->config.enableDepth) {
                pass->depthStencilAttachment = mfnull;
                continue;
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

            if(pass->depthStencilAttachment != mfnull) {
                u32 id = pass->depthStencilAttachment[0];
                pass->depthStencilAttachment = MF_ALLOCMEM(u32, sizeof(u32));
                pass->depthStencilAttachment[0] = id;
            }
        }
    }

    // TODO: Add MSAA support for rendergraph attachments!
    // Clear values
    {
        renderGraph->clearValues = MF_ALLOCMEM(VkClearValue, sizeof(VkClearValue) * config.attachmentCount);

        for(u32 i = 0; i < config.attachmentCount; i++) {
            MFRenderGraphAttachmentDesc* desc = &config.attachments[i];
            bool isDepth = mfFlagContainsBits(desc->type ,MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT);

            if(isDepth)
                renderGraph->clearValues->depthStencil.depth = 1.0f;
            else {
                renderGraph->clearValues[i].color.float32[0] = config.attachments[i].clearColor.r;
                renderGraph->clearValues[i].color.float32[1] = config.attachments[i].clearColor.g;
                renderGraph->clearValues[i].color.float32[2] = config.attachments[i].clearColor.b;
                renderGraph->clearValues[i].color.float32[3] = 1.0f;
            }
        }
    }
    // Attachments
    {
        renderGraph->igAttachmentSets = MF_ALLOCMEM(VkDescriptorSet, sizeof(VkDescriptorSet) * config.attachmentCount * FRAMES_IN_FLIGHT);
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
            
            VulkanImageCreate(&renderGraph->attachments[i], info);

            if(backend->config.enableUI) {
                for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                    renderGraph->igAttachmentSets[i * FRAMES_IN_FLIGHT + j] = ImGui_ImplVulkan_AddTexture(renderGraph->attachments[i].sampler, renderGraph->attachments[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }
        }
    }
    // RenderPass
    {
        renderGraph->passes = MF_ALLOCMEM(VkRenderPass, sizeof(VkRenderPass) * config.passCount);

        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &config.passes[i];
            u32 totalAttachments = pass->outputColorAttachmentCount;
            if(pass->depthStencilAttachment != mfnull)
                totalAttachments++;

            VkAttachmentDescription* attachments = MF_ALLOCMEM(VkAttachmentDescription, sizeof(VkAttachmentDescription) * totalAttachments);
            VkAttachmentReference* attachmentRefs = MF_ALLOCMEM(VkAttachmentReference, sizeof(VkAttachmentReference) * totalAttachments);
            for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                attachments[j].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[j].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                attachments[j].format = config.attachments[pass->outputColorAttachments[j]].format;
                attachments[j].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[j].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[j].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[j].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[j].samples = VK_SAMPLE_COUNT_1_BIT;

                attachmentRefs[j].attachment = j;
                attachmentRefs[j].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            if(pass->depthStencilAttachment != mfnull) {
                attachments[totalAttachments - 1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[totalAttachments - 1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                attachments[totalAttachments - 1].format = config.attachments[pass->depthStencilAttachment[0]].format;
                attachments[totalAttachments - 1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[totalAttachments - 1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[totalAttachments - 1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[totalAttachments - 1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[totalAttachments - 1].samples = VK_SAMPLE_COUNT_1_BIT;
                
                attachmentRefs[totalAttachments - 1].attachment = totalAttachments - 1;
                attachmentRefs[totalAttachments - 1].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            }
            
            VkSubpassDependency dependency = {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
            };

            VkSubpassDescription subpass = {
                .colorAttachmentCount = pass->outputColorAttachmentCount,
                .pColorAttachments = attachmentRefs,
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS
            };

            if(pass->depthStencilAttachment != mfnull)
                subpass.pDepthStencilAttachment = &attachmentRefs[totalAttachments - 1];

            VkRenderPassCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = totalAttachments,
                .pAttachments = attachments,
                .dependencyCount = 1,
                .pDependencies = &dependency,
                .subpassCount = 1,
                .pSubpasses = &subpass
            };

            VK_CHECK(vkCreateRenderPass(ctx->device, &info, ctx->allocator, &renderGraph->passes[i]));

            MF_FREEMEM(attachments);
            MF_FREEMEM(attachmentRefs);
        }
    }
    // Framebuffer
    {
        renderGraph->fbs = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * config.passCount * FRAMES_IN_FLIGHT);

        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &config.passes[i];
            u32 totalAttachments = pass->outputColorAttachmentCount;
            if(pass->depthStencilAttachment != mfnull)
                totalAttachments++;

            VkImageView* attachments = MF_ALLOCMEM(VkImageView, sizeof(VkImageView) * totalAttachments);
            for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                attachments[j] = renderGraph->attachments[pass->outputColorAttachments[j]].view;
            }
            if(pass->depthStencilAttachment != mfnull)
                attachments[totalAttachments - 1] = renderGraph->attachments[pass->depthStencilAttachment[0]].view;

            VkFramebufferCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .attachmentCount = totalAttachments,
                .pAttachments = attachments,
                .width = config.width,
                .height = config.height,
                .layers = 1,
                .renderPass = renderGraph->passes[i]
            };

            for(u32 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                VK_CHECK(vkCreateFramebuffer(ctx->device, &info, ctx->allocator, &renderGraph->fbs[i * FRAMES_IN_FLIGHT + j]));
            }

            MF_FREEMEM(attachments);
        }
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
    
    // Descriptor sets and layouts for all attachments
    {
        MFResourceSetBindings* bindings = MF_ALLOCMEM(MFResourceSetBindings, sizeof(MFResourceSetBindings) * config.attachmentCount);
        // Creating bindings
        for(u32 j = 0; j < config.attachmentCount; j++) {
            bindings[j].binding = j;
            bindings[j].description.descriptorCount = 1;
            bindings[j].description.descriptorType = MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[j].description.stageFlags = MF_SHADER_STAGE_FRAGMENT; // TODO: Make it configurable if required
        }

        // Create layout
        renderGraph->attachmentSetLayout = mfResourceSetLayoutCreate(config.attachmentCount, bindings, renderer);

        // Create sets
        {
            VkDescriptorPoolSize sizes[1] = {
                { MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES_IN_FLIGHT * config.attachmentCount }
            };

            renderGraph->attachmentSetPool = VulkanGpuResCreatePool(ctx, MF_ARRAYLEN(sizes), sizes, FRAMES_IN_FLIGHT);
            VulkanGpuResDescriptorPool* pool = &mfArrayGetElement(ctx->descriptorPools, VulkanGpuResDescriptorPool, renderGraph->attachmentSetPool);
        
            VkDescriptorSetAllocateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorSetCount = 1,
                .pSetLayouts = &renderGraph->attachmentSetLayout->layout,
                .descriptorPool = pool->pool
            };

            for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                VK_CHECK(vkAllocateDescriptorSets(ctx->device, &info, &renderGraph->attachmentSets[j]));
            }

            if(pool->allocatedSets == VULKAN_GPU_RES_MAX_DESCRIPTORS)
                pool->isFull = true;

            MF_FREEMEM(bindings);
        }

        // Update sets
        {
            VkDescriptorImageInfo* imgInfos = MF_ALLOCMEM(VkDescriptorImageInfo, sizeof(VkDescriptorImageInfo) * config.attachmentCount);

            for(u32 j = 0; j < config.attachmentCount; j++) {
                imgInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imgInfos[j].imageView = renderGraph->attachments[j].view;
                imgInfos[j].sampler = renderGraph->attachments[j].sampler;
            }

            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            };

            for(u32 j = 0; j < config.attachmentCount; j++) {
                write.dstBinding = j;
                write.pImageInfo = &imgInfos[j];
                for(u8 frameIdx = 0; frameIdx < FRAMES_IN_FLIGHT; frameIdx++) {
                    write.dstSet = renderGraph->attachmentSets[frameIdx];
                    vkUpdateDescriptorSets(ctx->device, 1, &write, 0, mfnull);
                }
            }

            MF_FREEMEM(imgInfos);
        }
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

    for(u32 j = 0; j < renderGraph->config.passCount; j++) {
        for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(ctx->device, renderGraph->fbs[j * FRAMES_IN_FLIGHT + i], ctx->allocator);
        }

        vkDestroyRenderPass(ctx->device, renderGraph->passes[j], ctx->allocator);
    }

    for(u32 i = 0; i < renderGraph->config.attachmentCount; i++) {
        VulkanImageDestroy(&renderGraph->attachments[i]);
    }

    for(u32 i = 0; i < renderGraph->config.passCount; i++) {
        MF_FREEMEM(renderGraph->config.passes[i].name);
        MF_FREEMEM(renderGraph->config.passes[i].outputColorAttachments);
        MF_FREEMEM(renderGraph->config.passes[i].depthStencilAttachment);
    }

    for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VulkanCommandBufferFree(ctx, renderGraph->commandBuffers[i], ctx->commandPool);
        vkDestroyFence(ctx->device, renderGraph->fences[i], ctx->allocator);
    }

    for(u32 i = 0; i < ctx->swapchainImageCount; i++) {
        vkDestroySemaphore(ctx->device, renderGraph->renderFinishedSemas[i], ctx->allocator);
    }

    if(backend->config.enableUI) {
        for(u32 i = 0; i < renderGraph->config.attachmentCount; i++) {
            for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                ImGui_ImplVulkan_RemoveTexture(renderGraph->igAttachmentSets[i * FRAMES_IN_FLIGHT + j]);
            }
        }
    }

    VulkanGpuResDescriptorPool* attachmentPool = &mfArrayGetElement(ctx->descriptorPools, VulkanGpuResDescriptorPool, renderGraph->attachmentSetPool);
    for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++)
        vkFreeDescriptorSets(ctx->device, attachmentPool->pool, 1, &renderGraph->attachmentSets[j]);
    mfResourceSetLayoutDestroy(&renderGraph->attachmentSetLayout);

    MF_FREEMEM(renderGraph->igAttachmentSets);
    MF_FREEMEM(renderGraph->passes);
    MF_FREEMEM(renderGraph->fbs);
    MF_FREEMEM(renderGraph->renderFinishedSemas);
    MF_FREEMEM(renderGraph->clearValues);
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

MFResourceSetLayout* mfRenderGraphGetAttachmentsSetLayout(MFRenderGraph* renderGraph) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    
    return renderGraph->attachmentSetLayout;
}

void mfRenderGraphBindAttachmentsSet(MFRenderGraph* renderGraph, u64 setIndex, MFPipeline* pipeline) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderGraph->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VkCommandBuffer buff = backend->commandBuffers[backend->frameIndex];
    if(backend->renderGraph != mfnull) {
        buff = backend->renderGraph->commandBuffers[backend->frameIndex];
    }

    VulkanPipeline* pipelineBackend = (VulkanPipeline*)mfPipelineGetBackend(pipeline);
    vkCmdBindDescriptorSets(buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineBackend->layout, setIndex, 1, &renderGraph->attachmentSets[backend->frameIndex], 0, mfnull);
}

const MFRenderGraphAttachmentDesc* mfRenderGraphGetAttachment(MFRenderGraph* renderGraph, u32 attachmentIdx) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(attachmentIdx >= renderGraph->config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
    
    return &renderGraph->config.attachments[attachmentIdx];
}

ImTextureID mfRenderGraphGetAttachmentImTextureID(MFRenderGraph* renderGraph, u32 attachmentIdx) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(attachmentIdx >= renderGraph->config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
    
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderGraph->renderer);

    if(!backend->config.enableUI)
        return mfnull;
    
    return (ImTextureID)renderGraph->igAttachmentSets[attachmentIdx * FRAMES_IN_FLIGHT + backend->frameIndex];
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