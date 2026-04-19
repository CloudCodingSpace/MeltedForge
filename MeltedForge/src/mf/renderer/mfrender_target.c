#ifdef __cplusplus
extern "C" {
#endif

#include "mfrender_target.h"

#include "core/mfcore.h"

#include "mfrenderer.h"
#include "vk/backend.h"
#include "vk/image.h"
#include "vk/fb.h"
#include "vk/renderpass.h"
#include "vk/command_buffer.h"
#include "vk/render_target.h"

#include <cimgui.h>
#include <cimgui_impl.h>

void mfRenderTargetCreate(MFRenderTarget* renderTarget, MFRenderer* renderer, bool hasDepth) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(renderTarget->init, mfGetLogger(), "The render target is already initialised!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    renderTarget->hasDepth = hasDepth;
    renderTarget->resizeCallback = mfnull;

    renderTarget->renderer = renderer;
    renderTarget->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    
    if(hasDepth && renderTarget->backend->enableDepth) {
        VulkanImageInfo info = {
            .ctx = &renderTarget->backend->ctx,
            .width = renderTarget->backend->ctx.swapchainExtent.width,
            .height = renderTarget->backend->ctx.swapchainExtent.height,
            .gpuResource = false,
            .pixels = mfnull,
            .format = renderTarget->backend->ctx.depthFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .type = VK_IMAGE_TYPE_2D,
            .arrayLayers = 1,
            .viewType = VK_IMAGE_VIEW_TYPE_2D
        };

        VulkanImageCreate(&renderTarget->depthImage, info);
    }

    {
        VulkanRenderPassInfo info = {
            .format = renderTarget->backend->ctx.swapchainFormat.format,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .hasDepth = hasDepth && renderTarget->backend->enableDepth,
            .renderTarget = true
        };

        renderTarget->renderPass = VulkanRenderPassCreate(&renderTarget->backend->ctx, info);
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
                .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .type = VK_IMAGE_TYPE_2D,
                .arrayLayers = 1,
                .viewType = VK_IMAGE_VIEW_TYPE_2D
            };

            VulkanImageCreate(&renderTarget->images[i], info);
        }

        u32 count = 1;
        VkImageView views[2] = {
            renderTarget->images[i].view
        };

        if(hasDepth && renderTarget->backend->enableDepth) {
            views[1] = renderTarget->depthImage.view;
            count++;
        }

        renderTarget->frameBuffers[i] = VulkanFbCreate(&renderTarget->backend->ctx, renderTarget->renderPass, count, views, renderTarget->backend->ctx.swapchainExtent);
        if(renderTarget->backend->enableUI)
            renderTarget->igSets[i] = ImGui_ImplVulkan_AddTexture(renderTarget->images[i].sampler, renderTarget->images[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        renderTarget->commandBuffers[i] = VulkanCommandBufferAllocate(&renderTarget->backend->ctx, renderTarget->backend->ctx.commandPool, true);
    }
    renderTarget->renderFinishedSemas = MF_ALLOCMEM(VkSemaphore, sizeof(VkSemaphore) * renderTarget->backend->ctx.swapchainImageCount);
    for(u32 i = 0; i < renderTarget->backend->ctx.swapchainImageCount; i++) {
        VkSemaphoreCreateInfo semaInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        VK_CHECK(vkCreateSemaphore(renderTarget->backend->ctx.device, &semaInfo, renderTarget->backend->ctx.allocator, &renderTarget->renderFinishedSemas[i]));
    }

    renderTarget->begun = false;
    renderTarget->init = true;
}

void mfRenderTargetDestroy(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VulkanCommandBufferFree(&renderTarget->backend->ctx, renderTarget->commandBuffers[i], renderTarget->backend->ctx.commandPool);
    }
    for(u32 i = 0; i < renderTarget->backend->ctx.swapchainImageCount; i++) {
        vkDestroySemaphore(renderTarget->backend->ctx.device, renderTarget->renderFinishedSemas[i], renderTarget->backend->ctx.allocator);
    }
    
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if(renderTarget->backend->enableUI)
            ImGui_ImplVulkan_RemoveTexture(renderTarget->igSets[i]);
        
        VulkanFbDestroy(&renderTarget->backend->ctx, renderTarget->frameBuffers[i]);
        VulkanImageDestroy(&renderTarget->images[i]);
    }

    if(renderTarget->hasDepth && renderTarget->backend->enableDepth)
        VulkanImageDestroy(&renderTarget->depthImage);
    
    VulkanRenderPassDestroy(&renderTarget->backend->ctx, renderTarget->renderPass);
    
    MF_FREEMEM(renderTarget->renderFinishedSemas);    
    MF_SETMEM(renderTarget, 0, sizeof(MFRenderTarget));
}

void mfRenderTargetResize(MFRenderTarget* renderTarget, MFVec2 extent) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    if(extent.x == 0 || extent.y == 0) {
        return;
    }

    VK_CHECK(vkDeviceWaitIdle(renderTarget->backend->ctx.device));
    
    // Deleting
    {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            if(renderTarget->backend->enableUI)
                ImGui_ImplVulkan_RemoveTexture(renderTarget->igSets[i]);
            
            VulkanFbDestroy(&renderTarget->backend->ctx, renderTarget->frameBuffers[i]);
            VulkanImageDestroy(&renderTarget->images[i]);
        }
        
        if(renderTarget->hasDepth && renderTarget->backend->enableDepth)
            VulkanImageDestroy(&renderTarget->depthImage);
        if(renderTarget->backend->enableUI)
            MF_SETMEM(renderTarget->igSets, 0, sizeof(VkDescriptorSet) * FRAMES_IN_FLIGHT);
        MF_SETMEM(renderTarget->frameBuffers, 0, sizeof(VkFramebuffer) * FRAMES_IN_FLIGHT);
        MF_SETMEM(renderTarget->images, 0, sizeof(VulkanImage) * FRAMES_IN_FLIGHT);
        MF_SETMEM(&renderTarget->depthImage, 0, sizeof(VulkanImage));
    }
    // Re-creating
    {
        if(renderTarget->hasDepth && renderTarget->backend->enableDepth) {
            VulkanImageInfo info = {
                .ctx = &renderTarget->backend->ctx,
                .width = extent.x,
                .height = extent.y,
                .gpuResource = false,
                .pixels = mfnull,
                .format = renderTarget->backend->ctx.depthFormat,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .type = VK_IMAGE_TYPE_2D,
                .arrayLayers = 1,
                .viewType = VK_IMAGE_VIEW_TYPE_2D
            };

            VulkanImageCreate(&renderTarget->depthImage, info);
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
                    .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    .type = VK_IMAGE_TYPE_2D,
                    .arrayLayers = 1,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D
                };

                VulkanImageCreate(&renderTarget->images[i], info);
            }

            u32 count = 1;
            VkImageView views[2] = {
                renderTarget->images[i].view
            };

            if(renderTarget->hasDepth && renderTarget->backend->enableDepth) {
                views[1] = renderTarget->depthImage.view;
                count++;
            }

            renderTarget->frameBuffers[i] = VulkanFbCreate(&renderTarget->backend->ctx, renderTarget->renderPass, count, views, (VkExtent2D){extent.x, extent.y});
            if(renderTarget->backend->enableUI)
                renderTarget->igSets[i] = ImGui_ImplVulkan_AddTexture(renderTarget->images[i].sampler, renderTarget->images[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    // Transitioning the images explicitly
    {
        VkCommandBuffer cmd = renderTarget->commandBuffers[0];
        VK_CHECK(vkResetCommandBuffer(cmd, 0));
        VulkanCommandBufferBegin(cmd);

        for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VkImageMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = renderTarget->images[i].image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, NULL, 0, NULL, 1, &barrier
            );
        }

        VulkanCommandBufferEnd(cmd);
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd
        };
        VK_CHECK(vkQueueSubmit(renderTarget->backend->ctx.queueData.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(renderTarget->backend->ctx.queueData.graphicsQueue));
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkResetCommandBuffer(renderTarget->commandBuffers[i], 0));
        VulkanCommandBufferBegin(renderTarget->commandBuffers[i]);
    }

    // Begin the renderPass
    {
        u32 count = 1;
        VkClearValue values[2] = {
            renderTarget->backend->clearColor
        };
        
        if(renderTarget->hasDepth && renderTarget->backend->enableDepth) {
            count++;
            values[1].depthStencil.depth = 1.0f;
            values[1].depthStencil.stencil = 0;
        }

        VkRenderPassBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .clearValueCount = count,
            .pClearValues = values,
            .renderArea = (VkRect2D){.extent = (VkExtent2D){renderTarget->images[0].info.width, renderTarget->images[0].info.height}, .offset = (VkOffset2D){0, 0}},
            .renderPass = renderTarget->renderPass,
            .framebuffer = renderTarget->frameBuffers[renderTarget->backend->frameIndex]
        }; 

        vkCmdBeginRenderPass(renderTarget->commandBuffers[renderTarget->backend->frameIndex], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    if(renderTarget->resizeCallback != mfnull) {
        renderTarget->resizeCallback(renderTarget->userData);
    }
}

void mfRenderTargetBegin(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    VkCommandBuffer commandBuffer = renderTarget->commandBuffers[renderTarget->backend->frameIndex];

    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
    VulkanCommandBufferBegin(commandBuffer);

    u32 count = 1;
    VkClearValue values[2] = {
        renderTarget->backend->clearColor
    };
    
    if(renderTarget->hasDepth && renderTarget->backend->enableDepth) {
        count++;
        values[1].depthStencil.depth = 1.0f;
        values[1].depthStencil.stencil = 0;
    }

    VkRenderPassBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = count,
        .pClearValues = values,
        .renderArea = (VkRect2D){.extent = (VkExtent2D){renderTarget->images[0].info.width, renderTarget->images[0].info.height}, .offset = (VkOffset2D){0, 0}},
        .renderPass = renderTarget->renderPass,
        .framebuffer = renderTarget->frameBuffers[renderTarget->backend->frameIndex]
    }; 

    vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    renderTarget->begun = true;
}

void mfRenderTargetEnd(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    if(!renderTarget->begun) {
        return;
    }

    VkCommandBuffer commandBuffer = renderTarget->commandBuffers[renderTarget->backend->frameIndex];

    vkCmdEndRenderPass(commandBuffer);
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

    VK_CHECK(vkQueueSubmit(renderTarget->backend->ctx.queueData.graphicsQueue, 1, &info, VK_NULL_HANDLE));
}

void mfRenderTargetSetResizeCallback(MFRenderTarget* renderTarget, void (*callback)(void* userData), void* userData) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");
    MF_PANIC_IF(userData == mfnull, mfGetLogger(), "The user data provided shouldn't be null!");
    MF_PANIC_IF(callback == mfnull, mfGetLogger(), "The resize callback func ptr provided shouldn't be null!");

    renderTarget->userData = userData;
    renderTarget->resizeCallback = callback;
}

void* mfRenderTargetGetPass(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    return (void*)renderTarget->renderPass;
}

u32 mfRenderTargetGetWidth(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    return renderTarget->images[0].info.width;
}

u32 mfRenderTargetGetHeight(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    return renderTarget->images[0].info.height;
}

ImTextureID mfRenderTargetGetImGuiTextureID(MFRenderTarget* renderTarget) {
    MF_PANIC_IF(renderTarget == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(!renderTarget->init, mfGetLogger(), "The render target isn't provided!");

    if(!renderTarget->backend->enableUI)
        return mfnull;
    return (ImTextureID)renderTarget->igSets[renderTarget->backend->frameIndex];
}

size_t mfRenderTargetGetSizeInBytes(void) {
    return sizeof(MFRenderTarget);
}

#ifdef __cplusplus
}
#endif