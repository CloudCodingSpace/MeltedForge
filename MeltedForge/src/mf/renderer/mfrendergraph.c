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

            if(i == config.passCount - 1)
                MF_PANIC_IF(pass->outputColorAttachmentCount == 0 || !pass->outputColorAttachments, mfGetLogger(), "Atleast the last pass of the rendergraph must have atleast one output attachment & so the output attachment's array mustn't be null!");

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
        renderGraph->clearValues = MF_ALLOCMEM(VkClearValue*, sizeof(VkClearValue*) * config.passCount);

        for(u32 k = 0; k < config.passCount; k++) {
            MFRenderGraphPassDesc* pass = &config.passes[k];
            u32 totalAttachments = pass->outputColorAttachmentCount;
            if(pass->depthStencilAttachment != mfnull)
                totalAttachments++;

            renderGraph->clearValues[k] = MF_ALLOCMEM(VkClearValue, sizeof(VkClearValue) * totalAttachments);

            for(u32 i = 0; i < pass->outputColorAttachmentCount; i++) {
                MFRenderGraphAttachmentDesc* desc = &config.attachments[pass->outputColorAttachments[i]];

                renderGraph->clearValues[k][i].color.float32[0] = config.attachments[i].clearColor.r;
                renderGraph->clearValues[k][i].color.float32[1] = config.attachments[i].clearColor.g;
                renderGraph->clearValues[k][i].color.float32[2] = config.attachments[i].clearColor.b;
                renderGraph->clearValues[k][i].color.float32[3] = 1.0f;
            }

            if(pass->depthStencilAttachment != mfnull) {
                renderGraph->clearValues[k][totalAttachments - 1].depthStencil.depth = 1.0f;
                renderGraph->clearValues[k][totalAttachments - 1].depthStencil.stencil = 0.0f;
            }
        }
    }
    // Attachments
    {
        renderGraph->igAttachmentSets = MF_ALLOCMEM(VkDescriptorSet, sizeof(VkDescriptorSet) * config.attachmentCount * FRAMES_IN_FLIGHT);
        renderGraph->attachments = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * config.attachmentCount * FRAMES_IN_FLIGHT);
        
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
            
            for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++)
                VulkanImageCreate(&renderGraph->attachments[i * FRAMES_IN_FLIGHT + j], info);

            if(backend->config.enableUI && !backend->config.headless) {
                for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                    renderGraph->igAttachmentSets[i * FRAMES_IN_FLIGHT + j] = ImGui_ImplVulkan_AddTexture(renderGraph->attachments[i * FRAMES_IN_FLIGHT + j].sampler, renderGraph->attachments[i * FRAMES_IN_FLIGHT + j].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }
        }
    }
    // RenderPass
    {
        renderGraph->passes = MF_ALLOCMEM(VkRenderPass, sizeof(VkRenderPass) * config.passCount);
        bool* attachmentUsedBefore = MF_ALLOCMEM(bool, sizeof(bool) * config.attachmentCount);

        for(u32 i = 0; i < config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &config.passes[i];
            u32 totalAttachments = pass->outputColorAttachmentCount;
            if(pass->depthStencilAttachment != mfnull)
                totalAttachments++;

            VkAttachmentDescription* attachments = MF_ALLOCMEM(VkAttachmentDescription, sizeof(VkAttachmentDescription) * totalAttachments);
            VkAttachmentReference* attachmentRefs = MF_ALLOCMEM(VkAttachmentReference, sizeof(VkAttachmentReference) * totalAttachments);
            for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                if(attachmentUsedBefore[pass->outputColorAttachments[j]]) {
                    attachments[j].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachments[j].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachments[j].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                } else {
                    attachments[j].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachments[j].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachments[j].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    attachmentUsedBefore[pass->outputColorAttachments[j]] = true;
                }
                attachments[j].format = config.attachments[pass->outputColorAttachments[j]].format;
                attachments[j].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[j].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[j].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[j].samples = VK_SAMPLE_COUNT_1_BIT;

                attachmentRefs[j].attachment = j;
                attachmentRefs[j].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            if(pass->depthStencilAttachment != mfnull) {
                u32 idx = totalAttachments - 1;

                if(attachmentUsedBefore[pass->depthStencilAttachment[0]]) {
                    attachments[idx].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachments[idx].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachments[idx].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                } else {
                    attachments[idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachments[idx].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachments[idx].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    attachmentUsedBefore[pass->depthStencilAttachment[0]] = true;
                }
                attachments[idx].format = config.attachments[pass->depthStencilAttachment[0]].format;
                attachments[idx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[idx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[idx].samples = VK_SAMPLE_COUNT_1_BIT;
                
                attachmentRefs[idx].attachment = idx;
                attachmentRefs[idx].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
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

            VkImageView* attachments = MF_ALLOCMEM(VkImageView, sizeof(VkImageView) * totalAttachments * FRAMES_IN_FLIGHT);
            for(u8 k = 0; k < FRAMES_IN_FLIGHT; k++) {
                for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                    attachments[k * totalAttachments + j] = renderGraph->attachments[pass->outputColorAttachments[j] * FRAMES_IN_FLIGHT + k].view;
                }
                
                if(pass->depthStencilAttachment != mfnull)
                    attachments[k * totalAttachments + totalAttachments - 1] = renderGraph->attachments[pass->depthStencilAttachment[0] * FRAMES_IN_FLIGHT + k].view;
            }

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
                info.pAttachments = &attachments[j * totalAttachments];
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
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            };

            for(u32 j = 0; j < config.attachmentCount; j++) {
                VkDescriptorImageInfo info = {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };

                write.dstBinding = j;
                write.pImageInfo = &info;
                for(u8 frameIdx = 0; frameIdx < FRAMES_IN_FLIGHT; frameIdx++) {
                    info.imageView = renderGraph->attachments[j * FRAMES_IN_FLIGHT + frameIdx].view;
                    info.sampler = renderGraph->attachments[j * FRAMES_IN_FLIGHT + frameIdx].sampler;
                    write.dstSet = renderGraph->attachmentSets[frameIdx];
                    vkUpdateDescriptorSets(ctx->device, 1, &write, 0, mfnull);
                }
            }
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
        for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++)
            VulkanImageDestroy(&renderGraph->attachments[i * FRAMES_IN_FLIGHT + j]);
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

    if(backend->config.enableUI && !backend->config.headless) {
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

    for(u32 i = 0; i < renderGraph->config.passCount; i++)
        MF_FREEMEM(renderGraph->clearValues[i]);
    MF_FREEMEM(renderGraph->clearValues);

    MF_FREEMEM(renderGraph->igAttachmentSets);
    MF_FREEMEM(renderGraph->passes);
    MF_FREEMEM(renderGraph->fbs);
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

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderGraph->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    MF_PANIC_IF(backend->renderGraph != mfnull, mfGetLogger(), "Another rendergaph musn't be invoked while another is already being invoked!");
    
    VkCommandBuffer buff = renderGraph->commandBuffers[backend->frameIndex];

    VK_CHECK(vkResetCommandBuffer(buff, 0));
    VulkanCommandBufferBegin(buff, true);

    backend->renderGraph = renderGraph;
    backend->ctx.hadRenderGraphUsage = true;
    // TODO: Make this search faster if required
    bool exists = false;
    for(u64 i = 0; i < backend->waitSemas.len; i++) {
        if(mfArrayGetElement(backend->waitStages, VkSemaphore, i) == renderGraph->renderFinishedSemas[backend->swapchainImageIndex])
            exists = true;
    }
    if(!exists) {
        mfArrayAddElement(&backend->waitSemas, VkSemaphore, renderGraph->renderFinishedSemas[backend->swapchainImageIndex]);
        mfArrayAddElement(&backend->waitStages, VkPipelineStageFlags, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    // TODO: Add querypools to each pass to get gpu timings
    // Actual recording of commands
    for(u32 i = 0; i < renderGraph->config.passCount; i++) {
        MFRenderGraphPassDesc* passDesc = &renderGraph->config.passes[i];
        u32 totalAttachment = passDesc->outputColorAttachmentCount;
        if(passDesc->depthStencilAttachment != mfnull)
            totalAttachment++;
   
        for(u32 k = 0; k < passDesc->outputColorAttachmentCount; k++) {
            renderGraph->attachments[passDesc->outputColorAttachments[k]].layout = VK_IMAGE_LAYOUT_UNDEFINED;
            renderGraph->attachments[passDesc->outputColorAttachments[k]].stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            renderGraph->attachments[passDesc->outputColorAttachments[k]].access = 0;
        }
        if(passDesc->depthStencilAttachment != mfnull) {
            renderGraph->attachments[passDesc->depthStencilAttachment[0]].layout = VK_IMAGE_LAYOUT_UNDEFINED;
            renderGraph->attachments[passDesc->depthStencilAttachment[0]].stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            renderGraph->attachments[passDesc->depthStencilAttachment[0]].access = 0;
        }

        VkRenderPassBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .clearValueCount = totalAttachment,
            .pClearValues = renderGraph->clearValues[i],
            .framebuffer = renderGraph->fbs[i * FRAMES_IN_FLIGHT + backend->frameIndex],
            .renderPass = renderGraph->passes[i],
            .renderArea = (VkRect2D) { .extent = { renderGraph->config.width, renderGraph->config.height }, .offset = { 0, 0 } }
        };

        vkCmdBeginRenderPass(buff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if(passDesc->passDrawCallback != mfnull)
            passDesc->passDrawCallback(passDesc->userData);

        vkCmdEndRenderPass(buff);

        for(u32 k = 0; k < passDesc->outputColorAttachmentCount; k++) {
            renderGraph->attachments[passDesc->outputColorAttachments[k]].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            renderGraph->attachments[passDesc->outputColorAttachments[k]].stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            renderGraph->attachments[passDesc->outputColorAttachments[k]].access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        if(passDesc->depthStencilAttachment != mfnull) {
            renderGraph->attachments[passDesc->depthStencilAttachment[0]].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            renderGraph->attachments[passDesc->depthStencilAttachment[0]].stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            renderGraph->attachments[passDesc->depthStencilAttachment[0]].access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
    }

    VulkanCommandBufferEnd(buff);

    VkPipelineStageFlagBits waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &buff,
        .pWaitDstStageMask = &waitStage,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &backend->imageAvailableSemas[backend->frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderGraph->renderFinishedSemas[backend->swapchainImageIndex]
    };

    if(backend->config.headless) {
        static u8 count = 1;
        if(count > ctx->swapchainImageCount)
            info.pWaitSemaphores = &backend->renderFinishedSemas[backend->swapchainImageIndex];
        else {
            info.waitSemaphoreCount = 0;
            count++;
        }
    }

    VkFence fence = renderGraph->fences[backend->frameIndex];
    VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &info, waitOnCpu ? fence : mfnull));
    if(waitOnCpu) {
        VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(ctx->device, 1, &fence));
    }

    backend->renderGraph = mfnull;
}

void mfRenderGraphResize(MFRenderGraph* renderGraph, u32 width, u32 height) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(width == 0 || height == 0, mfGetLogger(), "The width & height of the rendergraph extent musn't be null!");

    renderGraph->config.width = width;
    renderGraph->config.height = height;

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderGraph->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    MF_PANIC_IF(backend->renderGraph == renderGraph, mfGetLogger(), "The rendergraph musn't be resized while it is still being invoked!");

    VK_CHECK(vkQueueWaitIdle(ctx->queueData.graphicsQueue));

    // Deleting
    {
        for(u32 j = 0; j < renderGraph->config.passCount; j++) {
            for(u8 i = 0; i < FRAMES_IN_FLIGHT; i++) {
                vkDestroyFramebuffer(ctx->device, renderGraph->fbs[j * FRAMES_IN_FLIGHT + i], ctx->allocator);
            }
        }

        for(u32 i = 0; i < renderGraph->config.attachmentCount; i++) {
            for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++)
                VulkanImageDestroy(&renderGraph->attachments[i * FRAMES_IN_FLIGHT + j]);
        }

        if(backend->config.enableUI && !backend->config.headless) {
            for(u32 i = 0; i < renderGraph->config.attachmentCount; i++) {
                for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                    ImGui_ImplVulkan_RemoveTexture(renderGraph->igAttachmentSets[i * FRAMES_IN_FLIGHT + j]);
                }
            }
        }
    }
    // Recreating
    {
        
        for(u32 i = 0; i < renderGraph->config.attachmentCount; i++) {
            MFRenderGraphAttachmentDesc* desc = &renderGraph->config.attachments[i];
            bool isDepth = mfFlagContainsBits(desc->type ,MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT);

            VulkanImageInfo info = {
                .arrayLayers = 1,
                .aspectFlags = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                .ctx = ctx,
                .format = (VkFormat)(u32)desc->format,
                .gpuResource = true,
                .width = renderGraph->config.width,
                .height = renderGraph->config.height,
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
            
            for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++)
                VulkanImageCreate(&renderGraph->attachments[i * FRAMES_IN_FLIGHT + j], info);

            if(backend->config.enableUI && !backend->config.headless) {
                for(u8 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                    renderGraph->igAttachmentSets[i * FRAMES_IN_FLIGHT + j] = ImGui_ImplVulkan_AddTexture(renderGraph->attachments[i * FRAMES_IN_FLIGHT + j].sampler, renderGraph->attachments[i * FRAMES_IN_FLIGHT + j].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }
        }

        // Framebuffer
        for(u32 i = 0; i < renderGraph->config.passCount; i++) {
            MFRenderGraphPassDesc* pass = &renderGraph->config.passes[i];
            u32 totalAttachments = pass->outputColorAttachmentCount;
            if(pass->depthStencilAttachment != mfnull)
                totalAttachments++;

            VkImageView* attachments = MF_ALLOCMEM(VkImageView, sizeof(VkImageView) * totalAttachments * FRAMES_IN_FLIGHT);
            for(u8 k = 0; k < FRAMES_IN_FLIGHT; k++) {
                for(u32 j = 0; j < pass->outputColorAttachmentCount; j++) {
                    attachments[k * totalAttachments + j] = renderGraph->attachments[pass->outputColorAttachments[j] * FRAMES_IN_FLIGHT + k].view;
                }
                
                if(pass->depthStencilAttachment != mfnull)
                    attachments[k * totalAttachments + totalAttachments - 1] = renderGraph->attachments[pass->depthStencilAttachment[0] * FRAMES_IN_FLIGHT + k].view;
            }

            VkFramebufferCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .attachmentCount = totalAttachments,
                .pAttachments = attachments,
                .width = renderGraph->config.width,
                .height = renderGraph->config.height,
                .layers = 1,
                .renderPass = renderGraph->passes[i]
            };

            for(u32 j = 0; j < FRAMES_IN_FLIGHT; j++) {
                info.pAttachments = &attachments[j * totalAttachments];
                VK_CHECK(vkCreateFramebuffer(ctx->device, &info, ctx->allocator, &renderGraph->fbs[i * FRAMES_IN_FLIGHT + j]));
            }

            MF_FREEMEM(attachments);
        }

        // Updating the descriptor sets
        {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            };

            for(u32 j = 0; j < renderGraph->config.attachmentCount; j++) {
                VkDescriptorImageInfo info = {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };

                write.dstBinding = j;
                write.pImageInfo = &info;
                for(u8 frameIdx = 0; frameIdx < FRAMES_IN_FLIGHT; frameIdx++) {
                    info.imageView = renderGraph->attachments[j * FRAMES_IN_FLIGHT + frameIdx].view;
                    info.sampler = renderGraph->attachments[j * FRAMES_IN_FLIGHT + frameIdx].sampler;
                    write.dstSet = renderGraph->attachmentSets[frameIdx];
                    vkUpdateDescriptorSets(ctx->device, 1, &write, 0, mfnull);
                }
            }
        }
    }

    if(renderGraph->config.resizeCallback != mfnull) {
        renderGraph->config.resizeCallback(renderGraph->config.resizeCallbackUserState);
    }
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

u32 mfRenderGraphGetAttachmentBytesPerPixel(MFRenderGraph* renderGraph, u32 attachmentIdx) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(attachmentIdx >= renderGraph->config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");

    return VulkanFormatBytesPerPixel(renderGraph->attachments[attachmentIdx].info.format);
}

u8* mfRenderGraphGetAttachmentPixels(MFRenderGraph* renderGraph, u32 attachmentIdx, u32* width, u32* height) {
    MF_PANIC_IF(renderGraph == mfnull, mfGetLogger(), "The rendergraph handle provided shouldn't be null!");
    MF_PANIC_IF(!renderGraph->init, mfGetLogger(), "The rendergraph handle provided should have been initialised!");
    MF_PANIC_IF(attachmentIdx >= renderGraph->config.attachmentCount, mfGetLogger(), "The attachment's index reference is out of bounds of the total no. of attachments provided to the rendergraph!");
    MF_PANIC_IF(width == mfnull, mfGetLogger(), "The output width ptr shouldn't be null!");
    MF_PANIC_IF(height == mfnull, mfGetLogger(), "The output height ptr shouldn't be null!");

    return VulkanImageGetPixels(&renderGraph->attachments[attachmentIdx], 0, 0, width, height);
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

    if(!backend->config.enableUI || backend->config.headless)
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