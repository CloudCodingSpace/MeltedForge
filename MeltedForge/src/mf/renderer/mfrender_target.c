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

struct MFRenderTarget_s {
    MFRenderer* renderer;
    VulkanBackend* backend;

    VulkanImage* images;
    VkFramebuffer* fbs;
    VkRenderPass pass;
    VkDescriptorSet* descs;

    VkCommandBuffer buffs[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];

    b8 hasDepth;
};

void mfRenderTargetCreate(MFRenderTarget* rt, MFRenderer* renderer, b8 hasDepth) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    rt->hasDepth = hasDepth;

    rt->renderer = renderer;
    rt->backend = (VulkanBackend*)mfRendererGetBackend(renderer);

    rt->images = MF_ALLOCMEM(VulkanImage, sizeof(VulkanImage) * rt->backend->ctx.scImgCount);
    rt->fbs = MF_ALLOCMEM(VkFramebuffer, sizeof(VkFramebuffer) * rt->backend->ctx.scImgCount);
    rt->descs = MF_ALLOCMEM(VkDescriptorSet, sizeof(VkDescriptorSet) * rt->backend->ctx.scImgCount);
    
    rt->pass = VulkanRenderPassCreate(&rt->backend->ctx, rt->backend->ctx.scFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, hasDepth, true);
    
    for(u32 i = 0; i < rt->backend->ctx.scImgCount; i++) {
        VulkanImageCreate(&rt->images[i], &rt->backend->ctx, rt->backend->ctx.scExtent.width, 
                        rt->backend->ctx.scExtent.height, true, mfnull, 
                        rt->backend->ctx.scFormat.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                        VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        u32 count = 1;
        VkImageView views[2] = {
            rt->images[i].view
        };

        if(hasDepth) {
            views[1] = rt->backend->ctx.depthImage.view; //! FIXME: Per render-target must have it's own depth image with the appropriate dimension!!!!!!!!
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
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VulkanCommandBufferFree(&rt->backend->ctx, rt->buffs[i], rt->backend->ctx.cmdPool);
        vkDestroyFence(rt->backend->ctx.device, rt->fences[i], rt->backend->ctx.allocator);
    }
    
    for(u32 i = 0; i < rt->backend->ctx.scImgCount; i++) {
        ImGui_ImplVulkan_RemoveTexture(rt->descs[i]);
        
        VulkanFbDestroy(&rt->backend->ctx, rt->fbs[i]);
        VulkanImageDestroy(&rt->images[i], &rt->backend->ctx);
    }
    
    VulkanRenderPassDestroy(&rt->backend->ctx, rt->pass);
    
    MF_FREEMEM(rt->images);
    MF_FREEMEM(rt->fbs);
    MF_FREEMEM(rt->descs);
    
    MF_SETMEM(rt, 0, sizeof(MFRenderTarget));
}

void mfRenderTargetResize(MFRenderTarget* rt, MFVec2 extent) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    if(extent.x == 0 || extent.y == 0) {
        return;
    }

    VK_CHECK(vkDeviceWaitIdle(rt->backend->ctx.device));

    // Deleting
    {
        for(u32 i = 0; i < rt->backend->ctx.scImgCount; i++) {
            ImGui_ImplVulkan_RemoveTexture(rt->descs[i]);
            
            VulkanFbDestroy(&rt->backend->ctx, rt->fbs[i]);
            VulkanImageDestroy(&rt->images[i], &rt->backend->ctx);
        }
        
        VulkanRenderPassDestroy(&rt->backend->ctx, rt->pass);

        MF_SETMEM(rt->descs, 0, sizeof(VkDescriptorSet) * rt->backend->ctx.scImgCount);
        MF_SETMEM(rt->fbs, 0, sizeof(VkFramebuffer) * rt->backend->ctx.scImgCount);
        MF_SETMEM(rt->images, 0, sizeof(VulkanImage) * rt->backend->ctx.scImgCount);
    }
    // Re-creating
    {
        rt->pass = VulkanRenderPassCreate(&rt->backend->ctx, rt->backend->ctx.scFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rt->hasDepth, true);
    
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VK_CHECK(vkResetCommandBuffer(rt->buffs[i], 0));
            VulkanCommandBufferBegin(rt->buffs[i]);
        }

        for(u32 i = 0; i < rt->backend->ctx.scImgCount; i++) {
            VulkanImageCreate(&rt->images[i], &rt->backend->ctx, extent.x, 
                            extent.y, true, mfnull, 
                            rt->backend->ctx.scFormat.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                            VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            u32 count = 1;
            VkImageView views[2] = {
                rt->images[i].view
            };

            if(rt->hasDepth) {
                views[1] = rt->backend->ctx.depthImage.view;
                count++;
            }

            rt->fbs[i] = VulkanFbCreate(&rt->backend->ctx, rt->pass, count, views, (VkExtent2D){extent.x, extent.y});

            rt->descs[i] = ImGui_ImplVulkan_AddTexture(rt->images[i].sampler, rt->images[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
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
            .renderArea = (VkRect2D){.extent = (VkExtent2D){rt->images[0].width, rt->images[0].height}, .offset = (VkOffset2D){0, 0}},
            .renderPass = rt->pass,
            .framebuffer = rt->fbs[rt->backend->scImgIdx]
        }; 

        vkCmdBeginRenderPass(rt->buffs[rt->backend->crntFrmIdx], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
}

void mfRenderTargetBegin(MFRenderTarget* rt) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
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
        .renderArea = (VkRect2D){.extent = (VkExtent2D){rt->images[0].width, rt->images[0].height}, .offset = (VkOffset2D){0, 0}},
        .renderPass = rt->pass,
        .framebuffer = rt->fbs[rt->backend->scImgIdx]
    }; 

    vkCmdBeginRenderPass(buff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void mfRenderTargetEnd(MFRenderTarget* rt) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
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

u32 mfRenderTargetGetWidth(MFRenderTarget* rt) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    return rt->images[0].width;
}

u32 mfRenderTargetGetHeight(MFRenderTarget* rt) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");
    
    return rt->images[0].height;
}

void* mfRenderTargetGetHandle(MFRenderTarget* rt) {
    MF_ASSERT(rt == mfnull, mfGetLogger(), "The render target handle provided shouldn't be null!");

    return rt->descs[rt->backend->scImgIdx];
}

size_t mfGetRenderTargetSizeInBytes() {
    return sizeof(MFRenderTarget);
}