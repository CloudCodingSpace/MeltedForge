#ifdef __cplusplus
extern "C" {
#endif

#include "rendertarget.h"

#include "core/mfutils.h"

#include "backend.h"
#include "common.h"
#include "ctx.h"
#include "image.h"
#include "command_buffer.h"

void VulkanRenderTargetCreate(VulkanRenderTarget* renderTarget, MFRenderer* renderer, bool hasDepth) {
    renderTarget->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    renderTarget->clearValue = renderTarget->backend->clearColor;
    renderTarget->samples = renderTarget->backend->ctx.samples;
    renderTarget->hasMsaa = renderTarget->backend->ctx.samples != VK_SAMPLE_COUNT_1_BIT;
    renderTarget->hasDepth = hasDepth && renderTarget->backend->config.enableDepth;

    if(renderTarget->hasDepth) {
        VulkanImageInfo info = {
            .ctx = &renderTarget->backend->ctx,
            .width = renderTarget->backend->ctx.swapchainExtent.width,
            .height = renderTarget->backend->ctx.swapchainExtent.height,
            .gpuResource = true,
            .pixels = mfnull,
            .format = renderTarget->backend->depthFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
            .type = VK_IMAGE_TYPE_2D,
            .arrayLayers = 1,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .samples = renderTarget->samples,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST
        };

        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
            VulkanImageCreate(&renderTarget->depthImages[i], info);
    }

    {
        VulkanRenderPassInfo info = {
            .format = renderTarget->backend->ctx.swapchainFormat.format,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .hasDepth = renderTarget->hasDepth,
            .hasMsaa = renderTarget->hasMsaa
        };

        VulkanRenderPassCreate(&renderTarget->renderPass, renderTarget->backend, info);
    }

    {
        MFResourceDescription desc = {
            .descriptorCount = 1,
            .descriptorType = MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = MF_SHADER_STAGE_FRAGMENT // TODO: Make it configurable if required
        };

        MFResourceSetBindings bindings[2] = {0};
        // Color attachment
        bindings[0].description = desc;
        bindings[0].binding = 0;
        bindings[1].description = desc;
        bindings[1].binding = 1;

        renderTarget->layout = mfResourceSetLayoutCreate(renderTarget->hasDepth ? 2 : 1, bindings, renderer);
        VulkanGpuResDescriptorPool* pool = mfnull;
        // Descriptor pool
        {
            VkDescriptorPoolSize sizes[1] = {
                { MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES_IN_FLIGHT * 2 }
            };

            u64 poolIdx = VulkanGpuResCreatePool(&renderTarget->backend->ctx, MF_ARRAYLEN(sizes), sizes, FRAMES_IN_FLIGHT);
            pool = &mfArrayGetElement(renderTarget->backend->ctx.descriptorPools, VulkanGpuResDescriptorPool, poolIdx);
        }

        VkDescriptorSetAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorSetCount = 1,
            .pSetLayouts = &renderTarget->layout->layout,
            .descriptorPool = pool->pool
        };
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VK_CHECK(vkAllocateDescriptorSets(renderTarget->backend->ctx.device, &info, &renderTarget->sets[i]));
        }

        if(pool->allocatedSets == VULKAN_GPU_RES_MAX_DESCRIPTORS)
            pool->isFull = true;
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        {
            VulkanImageInfo info = {
                .ctx = &renderTarget->backend->ctx,
                .width = renderTarget->backend->ctx.swapchainExtent.width,
                .height = renderTarget->backend->ctx.swapchainExtent.height,
                .gpuResource = true,
                .pixels = mfnull,
                .format = renderTarget->backend->ctx.swapchainFormat.format,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                .type = VK_IMAGE_TYPE_2D,
                .arrayLayers = 1,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST
            };

            VulkanImageCreate(&renderTarget->images[i], info);
            info.samples = renderTarget->samples;
            if(renderTarget->hasMsaa)
                VulkanImageCreate(&renderTarget->msaaImages[i], info);
        }

        {
            u32 count = 1;
            VulkanImage* attachments[3] = {
                &renderTarget->images[i]
            };

            if(renderTarget->hasDepth) {
                attachments[count++] = &renderTarget->depthImages[i];
            }
            if(renderTarget->hasMsaa)
                attachments[count++] = &renderTarget->msaaImages[i];

            VulkanFramebufferCreate(&renderTarget->frameBuffers[i], &renderTarget->backend->ctx, renderTarget->renderPass.handle, count, attachments, renderTarget->backend->ctx.swapchainExtent);
        }

        if(renderTarget->backend->config.enableUI) {
            VkSampler sampler = renderTarget->hasMsaa ? renderTarget->msaaImages[i].sampler : renderTarget->images[i].sampler;
            VkImageView view = renderTarget->hasMsaa ? renderTarget->msaaImages[i].view : renderTarget->images[i].view;
            renderTarget->igColorSets[i] = ImGui_ImplVulkan_AddTexture(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        {
            VkDescriptorImageInfo imgInfo = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = renderTarget->images[i].view,
                .sampler = renderTarget->images[i].sampler
            };

            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .dstBinding = 0,
                .dstSet = renderTarget->sets[i],
                .pImageInfo = &imgInfo
            };

            vkUpdateDescriptorSets(renderTarget->backend->ctx.device, 1, &write, 0, mfnull);
            
            imgInfo.imageView = renderTarget->depthImages[i].view;
            imgInfo.sampler = renderTarget->depthImages[i].sampler;
            write.dstBinding = 1;
            if(renderTarget->hasDepth)
                vkUpdateDescriptorSets(renderTarget->backend->ctx.device, 1, &write, 0, mfnull);
        }

        renderTarget->commandBuffers[i] = VulkanCommandBufferAllocate(&renderTarget->backend->ctx, renderTarget->backend->ctx.commandPool, true);
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkFenceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };
        VK_CHECK(vkCreateFence(renderTarget->backend->ctx.device, &info, renderTarget->backend->ctx.allocator, &renderTarget->fences[i]));
        VK_CHECK(vkResetFences(renderTarget->backend->ctx.device, 1, &renderTarget->fences[i]));
    }

    renderTarget->renderFinishedSemas = MF_ALLOCMEM(VkSemaphore, sizeof(VkSemaphore) * renderTarget->backend->ctx.swapchainImageCount);
    for(u32 i = 0; i < renderTarget->backend->ctx.swapchainImageCount; i++) {
        VkSemaphoreCreateInfo semaInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        VK_CHECK(vkCreateSemaphore(renderTarget->backend->ctx.device, &semaInfo, renderTarget->backend->ctx.allocator, &renderTarget->renderFinishedSemas[i]));
    }
}

void VulkanRenderTargetDestroy(VulkanRenderTarget* renderTarget) {
    for(u32 i = 0; i < renderTarget->backend->ctx.swapchainImageCount; i++) {
        vkDestroySemaphore(renderTarget->backend->ctx.device, renderTarget->renderFinishedSemas[i], renderTarget->backend->ctx.allocator);
    }
    
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(renderTarget->backend->ctx.device, renderTarget->fences[i], renderTarget->backend->ctx.allocator);

        VulkanCommandBufferFree(&renderTarget->backend->ctx, renderTarget->commandBuffers[i], renderTarget->backend->ctx.commandPool);
        if(renderTarget->backend->config.enableUI) {
            ImGui_ImplVulkan_RemoveTexture(renderTarget->igColorSets[i]);
        }

        VulkanFramebufferDestroy(&renderTarget->frameBuffers[i]);
        VulkanImageDestroy(&renderTarget->images[i]);
        if(renderTarget->hasMsaa)
            VulkanImageDestroy(&renderTarget->msaaImages[i]);
    }

    if(renderTarget->hasDepth) {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
            VulkanImageDestroy(&renderTarget->depthImages[i]);
    }

    mfResourceSetLayoutDestroy(&renderTarget->layout);
    VulkanRenderPassDestroy(&renderTarget->renderPass);

    MF_FREEMEM(renderTarget->renderFinishedSemas);
    MF_SETMEM(renderTarget, 0, sizeof(VulkanRenderTarget));
}

void VulkanRenderTargetResize(VulkanRenderTarget* renderTarget, MFVec2 extent) {
    VK_CHECK(vkDeviceWaitIdle(renderTarget->backend->ctx.device));
    
    // Deleting
    {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            if(renderTarget->backend->config.enableUI) {
                ImGui_ImplVulkan_RemoveTexture(renderTarget->igColorSets[i]);
            }
            
            VulkanFramebufferDestroy(&renderTarget->frameBuffers[i]);
            VulkanImageDestroy(&renderTarget->images[i]);
            if(renderTarget->hasMsaa)
                VulkanImageDestroy(&renderTarget->msaaImages[i]);
        }
        
        if(renderTarget->hasDepth) {
            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
                VulkanImageDestroy(&renderTarget->depthImages[i]);
        }
        if(renderTarget->backend->config.enableUI) {
            MF_SETMEM(renderTarget->igColorSets, 0, sizeof(VkDescriptorSet) * FRAMES_IN_FLIGHT);
        }
        MF_SETMEM(renderTarget->frameBuffers, 0, sizeof(VkFramebuffer) * FRAMES_IN_FLIGHT);
        MF_SETMEM(renderTarget->images, 0, sizeof(VulkanImage) * FRAMES_IN_FLIGHT);
        MF_SETMEM(renderTarget->msaaImages, 0, sizeof(VulkanImage) * FRAMES_IN_FLIGHT);
        MF_SETMEM(renderTarget->depthImages, 0, sizeof(VulkanImage) * FRAMES_IN_FLIGHT);
    }
    // Re-creating
    {
        if(renderTarget->hasDepth) {
            VulkanImageInfo info = {
                .ctx = &renderTarget->backend->ctx,
                .width = extent.x,
                .height = extent.y,
                .gpuResource = true,
                .pixels = mfnull,
                .format = renderTarget->backend->depthFormat,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                .type = VK_IMAGE_TYPE_2D,
                .arrayLayers = 1,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .samples = renderTarget->samples,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST
            };

            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
                VulkanImageCreate(&renderTarget->depthImages[i], info);
        }

        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            {
                VulkanImageInfo info = {
                    .ctx = &renderTarget->backend->ctx,
                    .width = extent.x,
                    .height = extent.y,
                    .gpuResource = true,
                    .pixels = mfnull,
                    .format = renderTarget->backend->ctx.swapchainFormat.format,
                    .tiling = VK_IMAGE_TILING_OPTIMAL,
                    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                    .memFlags = VMA_MEMORY_USAGE_GPU_ONLY,
                    .type = VK_IMAGE_TYPE_2D,
                    .arrayLayers = 1,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .magFilter = VK_FILTER_NEAREST,
                    .minFilter = VK_FILTER_NEAREST
                };

                VulkanImageCreate(&renderTarget->images[i], info);
                info.samples = renderTarget->samples;
                if(renderTarget->hasMsaa)
                    VulkanImageCreate(&renderTarget->msaaImages[i], info);
            }

            u32 count = 1;
            VulkanImage* attachments[3] = {
                &renderTarget->images[i]
            };

            if(renderTarget->hasDepth) {
                attachments[count++] = &renderTarget->depthImages[i];
            }
            if(renderTarget->hasMsaa)
                attachments[count++] = &renderTarget->msaaImages[i];

            VulkanFramebufferCreate(&renderTarget->frameBuffers[i], &renderTarget->backend->ctx, renderTarget->renderPass.handle, count, attachments, (VkExtent2D){extent.x, extent.y});
            if(renderTarget->backend->config.enableUI) {
                VkSampler sampler = renderTarget->hasMsaa ? renderTarget->msaaImages[i].sampler : renderTarget->images[i].sampler;
                VkImageView view = renderTarget->hasMsaa ? renderTarget->msaaImages[i].view : renderTarget->images[i].view;
                renderTarget->igColorSets[i] = ImGui_ImplVulkan_AddTexture(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            {
                VkDescriptorImageInfo imgInfo = {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = renderTarget->images[i].view,
                    .sampler = renderTarget->images[i].sampler
                };

                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .dstBinding = 0,
                    .dstSet = renderTarget->sets[i],
                    .pImageInfo = &imgInfo
                };
                vkUpdateDescriptorSets(renderTarget->backend->ctx.device, 1, &write, 0, mfnull);

                imgInfo.imageView = renderTarget->depthImages[i].view;
                imgInfo.sampler = renderTarget->depthImages[i].sampler;
                write.dstBinding = 1;
                if(renderTarget->hasDepth)
                    vkUpdateDescriptorSets(renderTarget->backend->ctx.device, 1, &write, 0, mfnull);
            }
        }
    }
}

void VulkanRenderTargetBegin(VulkanRenderTarget* renderTarget) {
    VkCommandBuffer commandBuffer = renderTarget->commandBuffers[renderTarget->backend->frameIndex];

    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
    VulkanCommandBufferBegin(commandBuffer, true);

    u32 count = 1;
    VkClearValue values[3] = {
        renderTarget->clearValue
    };
    
    if(renderTarget->hasDepth) {
        values[count].depthStencil.depth = 1.0f;
        values[count++].depthStencil.stencil = 0;
    }
    if(renderTarget->hasMsaa)
        values[count++] = values[0];


    VulkanRenderPassBeginInfo beginInfo = {
        .clearValueCount = count,
        .clearValues = values,
        .cmdBuff = commandBuffer,
        .extent = (VkRect2D){.extent = (VkExtent2D){renderTarget->images[0].info.width, renderTarget->images[0].info.height}, .offset = (VkOffset2D){0, 0}},
        .fb = &renderTarget->frameBuffers[renderTarget->backend->frameIndex]
    };

    VulkanRenderPassBegin(&renderTarget->renderPass, beginInfo);

    // TODO: Make this search faster if required
    bool exists = false;
    for(u64 i = 0; i < renderTarget->backend->waitSemas.len; i++) {
        if(mfArrayGetElement(renderTarget->backend->waitStages, VkSemaphore, i) == renderTarget->renderFinishedSemas[renderTarget->backend->swapchainImageIndex])
            exists = true;
    }
    if(!exists) {
        mfArrayAddElement(&renderTarget->backend->waitSemas, VkSemaphore, renderTarget->renderFinishedSemas[renderTarget->backend->swapchainImageIndex]);
        mfArrayAddElement(&renderTarget->backend->waitStages, VkPipelineStageFlags, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    renderTarget->backend->renderTarget = renderTarget;
    renderTarget->backend->ctx.hadRenderTargetUsage = true;
}

void VulkanRenderTargetEnd(VulkanRenderTarget* renderTarget, bool waitOnCpu) {
    
    VkCommandBuffer commandBuffer = renderTarget->commandBuffers[renderTarget->backend->frameIndex];

    VulkanRenderPassEnd(&renderTarget->renderPass, commandBuffer, &renderTarget->frameBuffers[renderTarget->backend->frameIndex]);
    VulkanCommandBufferEnd(commandBuffer);

    VkPipelineStageFlags waitDstFlags[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore waitSemas[1] = {
        renderTarget->backend->imageAvailableSemas[renderTarget->backend->frameIndex]
    };

    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderTarget->renderFinishedSemas[renderTarget->backend->swapchainImageIndex],
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemas,
        .pWaitDstStageMask = waitDstFlags
    };

    if(renderTarget->backend->config.headless) {
        static u8 count = 1;
        if(count > renderTarget->backend->ctx.swapchainImageCount)
            waitSemas[0] = renderTarget->backend->renderFinishedSemas[renderTarget->backend->swapchainImageIndex];
        else {
            info.waitSemaphoreCount = 0;
            count++;
        }
    }

    VkFence fence = renderTarget->fences[renderTarget->backend->frameIndex];
    VK_CHECK(vkQueueSubmit(renderTarget->backend->ctx.queueData.graphicsQueue, 1, &info, waitOnCpu ? fence : VK_NULL_HANDLE));
    if(waitOnCpu) {
        VK_CHECK(vkWaitForFences(renderTarget->backend->ctx.device, 1, &fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(renderTarget->backend->ctx.device, 1, &fence));
    }

    renderTarget->backend->renderTarget = mfnull;
}

#ifdef __cplusplus
}
#endif