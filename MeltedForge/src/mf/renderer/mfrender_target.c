#include "mfrender_target.h"

#include "core/mfcore.h"

#include "mfrenderer.h"
#include "vk/backend.h"
#include "vk/image.h"
#include "vk/fb.h"
#include "vk/renderpass.h"
#include "vk/cmd.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <cimgui_impl.h>

#include "vk/render_target.h"

void mfRenderTargetCreate(MFRenderTarget* rt, MFRenderer* renderer, b8 hasDepth) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    rt->hasDepth = hasDepth;
    rt->resizeCallback = mfnull;

    rt->renderer = renderer;
    rt->backend = (VulkanBackend*)mfRendererGetBackend(renderer);

    rt->images = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * FRAMES_IN_FLIGHT);
    rt->fbs = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * FRAMES_IN_FLIGHT);
    rt->descs = MF_ALLOCMEM(VkDescriptorSet, sizeof(VkDescriptorSet) * FRAMES_IN_FLIGHT);
    
    {
        VulkanImageInfo info = {
            .ctx = &rt->backend->ctx,
            .width = rt->backend->ctx.scExtent.width,
            .height = rt->backend->ctx.scExtent.height,
            .gpuResource = false,
            .pixels = mfnull,
            .format = rt->backend->ctx.depthFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .type = VK_IMAGE_TYPE_2D,
            .arrayLayers = 1,
            .viewType = VK_IMAGE_VIEW_TYPE_2D
        };

        VulkanImageCreate(&rt->depthImage, info);
    }
    {
        VulkanRenderPassInfo info = {
            .format = rt->backend->ctx.scFormat.format,
            .initiaLay = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLay = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .hasDepth = hasDepth,
            .renderTarget = true
        };

        rt->pass = VulkanRenderPassCreate(&rt->backend->ctx, info);
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        {
            VulkanImageInfo info = {
                .ctx = &rt->backend->ctx,
                .width = rt->backend->ctx.scExtent.width,
                .height = rt->backend->ctx.scExtent.height,
                .gpuResource = true,
                .pixels = mfnull,
                .format = rt->backend->ctx.scFormat.format,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .type = VK_IMAGE_TYPE_2D,
                .arrayLayers = 1,
                .viewType = VK_IMAGE_VIEW_TYPE_2D
            };

            VulkanImageCreate(&rt->images[i], info);
        }

        u32 count = 1;
        VkImageView views[2] = {
            rt->images[i].view
        };

        if(hasDepth) {
            views[1] = rt->depthImage.view;
            count++;
        }

        rt->fbs[i] = VulkanFbCreate(&rt->backend->ctx, rt->pass, count, views, rt->backend->ctx.scExtent);

        rt->descs[i] = ImGui_ImplVulkan_AddTexture(rt->images[i].sampler, rt->images[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        rt->buffs[i] = VulkanCommandBufferAllocate(&rt->backend->ctx, rt->backend->ctx.cmdPool, true);
        
        VkFenceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        VK_CHECK(vkCreateFence(rt->backend->ctx.device, &info, rt->backend->ctx.allocator, &rt->fences[i]));
    }
}

void mfRenderTargetDestroy(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VulkanCommandBufferFree(&rt->backend->ctx, rt->buffs[i], rt->backend->ctx.cmdPool);
        vkDestroyFence(rt->backend->ctx.device, rt->fences[i], rt->backend->ctx.allocator);
    }
    
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        ImGui_ImplVulkan_RemoveTexture(rt->descs[i]);
        
        VulkanFbDestroy(&rt->backend->ctx, rt->fbs[i]);
        VulkanImageDestroy(&rt->images[i]);
    }

    VulkanImageDestroy(&rt->depthImage);
    
    VulkanRenderPassDestroy(&rt->backend->ctx, rt->pass);
    
    MF_FREEMEM(rt->images);
    MF_FREEMEM(rt->fbs);
    MF_FREEMEM(rt->descs);
    
    MF_SETMEM(rt, 0, sizeof(MFRenderTarget));
}

void mfRenderTargetResize(MFRenderTarget* rt, MFVec2 extent) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    if(extent.x == 0 || extent.y == 0) {
        return;
    }

    VK_CHECK(vkDeviceWaitIdle(rt->backend->ctx.device));
    
    // Deleting
    {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            ImGui_ImplVulkan_RemoveTexture(rt->descs[i]);
            
            VulkanFbDestroy(&rt->backend->ctx, rt->fbs[i]);
            VulkanImageDestroy(&rt->images[i]);
        }
        
        VulkanImageDestroy(&rt->depthImage);
        
        MF_SETMEM(rt->descs, 0, sizeof(VkDescriptorSet) * FRAMES_IN_FLIGHT);
        MF_SETMEM(rt->fbs, 0, sizeof(VkFramebuffer) * FRAMES_IN_FLIGHT);
        MF_SETMEM(rt->images, 0, sizeof(VulkanImage) * FRAMES_IN_FLIGHT);
        MF_SETMEM(&rt->depthImage, 0, sizeof(VulkanImage));
    }
    // Re-creating
    {
        {
            VulkanImageInfo info = {
                .ctx = &rt->backend->ctx,
                .width = extent.x,
                .height = extent.y,
                .gpuResource = false,
                .pixels = mfnull,
                .format = rt->backend->ctx.depthFormat,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .type = VK_IMAGE_TYPE_2D,
                .arrayLayers = 1,
                .viewType = VK_IMAGE_VIEW_TYPE_2D
            };

            VulkanImageCreate(&rt->depthImage, info);
        }


        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            {
                VulkanImageInfo info = {
                    .ctx = &rt->backend->ctx,
                    .width = extent.x,
                    .height = extent.y,
                    .gpuResource = true,
                    .pixels = mfnull,
                    .format = rt->backend->ctx.scFormat.format,
                    .tiling = VK_IMAGE_TILING_OPTIMAL,
                    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                    .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    .type = VK_IMAGE_TYPE_2D,
                    .arrayLayers = 1,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D
                };

                VulkanImageCreate(&rt->images[i], info);
            }

            u32 count = 1;
            VkImageView views[2] = {
                rt->images[i].view
            };

            if(rt->hasDepth) {
                views[1] = rt->depthImage.view;
                count++;
            }

            rt->fbs[i] = VulkanFbCreate(&rt->backend->ctx, rt->pass, count, views, (VkExtent2D){extent.x, extent.y});

            rt->descs[i] = ImGui_ImplVulkan_AddTexture(rt->images[i].sampler, rt->images[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    if(rt->resizeCallback != mfnull) {
        rt->resizeCallback(rt->userData);
    }

    // Transitioning the images explicitly
    {
        VkCommandBuffer cmd = rt->buffs[0];
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
                .image = rt->images[i].image,
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
        VK_CHECK(vkQueueSubmit(rt->backend->ctx.qData.gQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(rt->backend->ctx.qData.gQueue));
    }

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkResetCommandBuffer(rt->buffs[i], 0));
        VulkanCommandBufferBegin(rt->buffs[i]);
    }

    // Begin the pass
    {
        u32 count = 1;
        VkClearValue values[2] = {
            rt->backend->clearColor
        };
        
        if(rt->hasDepth) {
            count++;
            values[1].depthStencil.depth = 1.0f;
            values[1].depthStencil.stencil = 0;
        }

        VkRenderPassBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .clearValueCount = count,
            .pClearValues = values,
            .renderArea = (VkRect2D){.extent = (VkExtent2D){rt->images[0].info.width, rt->images[0].info.height}, .offset = (VkOffset2D){0, 0}},
            .renderPass = rt->pass,
            .framebuffer = rt->fbs[rt->backend->crntFrmIdx]
        }; 

        vkCmdBeginRenderPass(rt->buffs[rt->backend->crntFrmIdx], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
}

void mfRenderTargetBegin(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    VkCommandBuffer buff = rt->buffs[rt->backend->crntFrmIdx];

    VK_CHECK(vkResetCommandBuffer(buff, 0));
    VulkanCommandBufferBegin(buff);

    u32 count = 1;
    VkClearValue values[2] = {
        rt->backend->clearColor
    };
    
    if(rt->hasDepth) {
        count++;
        values[1].depthStencil.depth = 1.0f;
        values[1].depthStencil.stencil = 0;
    }

    VkRenderPassBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = count,
        .pClearValues = values,
        .renderArea = (VkRect2D){.extent = (VkExtent2D){rt->images[0].info.width, rt->images[0].info.height}, .offset = (VkOffset2D){0, 0}},
        .renderPass = rt->pass,
        .framebuffer = rt->fbs[rt->backend->crntFrmIdx]
    }; 

    vkCmdBeginRenderPass(buff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void mfRenderTargetEnd(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    VkCommandBuffer buff = rt->buffs[rt->backend->crntFrmIdx];

    vkCmdEndRenderPass(buff);
    VulkanCommandBufferEnd(buff);

    VkPipelineStageFlags waitDstFlags[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &buff,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &rt->backend->rndrFinishedSemas[rt->backend->crntFrmIdx],
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &rt->backend->imgAvailableSemas[rt->backend->crntFrmIdx],
        .pWaitDstStageMask = waitDstFlags
    };

    VK_CHECK(vkQueueSubmit(rt->backend->ctx.qData.gQueue, 1, &info, rt->fences[rt->backend->crntFrmIdx]));
}

void mfRenderTargetSetResizeCallback(MFRenderTarget* rt, void (*callback)(void* userData), void* userData) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_PANIC_IF(userData == mfnull, mfGetLogger(), "The user data provided shouldn't be null!");
    MF_PANIC_IF(callback == mfnull, mfGetLogger(), "The resize callback func ptr provided shouldn't be null!");

    rt->userData = userData;
    rt->resizeCallback = callback;
}

void* mfRenderTargetGetPass(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");

    return (void*)rt->pass;
}

u32 mfRenderTargetGetWidth(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    return rt->images[0].info.width;
}

u32 mfRenderTargetGetHeight(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    return rt->images[0].info.height;
}

void* mfRenderTargetGetHandle(MFRenderTarget* rt) {
    MF_PANIC_IF(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");

    return rt->descs[rt->backend->crntFrmIdx];
}

size_t mfGetRenderTargetSizeInBytes() {
    return sizeof(MFRenderTarget);
}
