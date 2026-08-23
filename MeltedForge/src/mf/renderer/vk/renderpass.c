#ifdef __cplusplus
extern "C" {
#endif

#include "renderpass.h"
#include "backend.h"
#include "common.h"

void VulkanRenderPassCreate(VulkanRenderPass* pass, VulkanBackend* backend, VulkanRenderPassInfo pinfo) {
    pass->backend = backend;
    pass->ctx = &backend->ctx;
    pass->info = pinfo;

    VkAttachmentDescription colorAttachment = {
        .format = pinfo.format,
        .initialLayout = pinfo.initialLayout,
        .finalLayout = pinfo.finalLayout,
        .samples = pinfo.hasMsaa ? pass->ctx->samples : VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE
    };

    VkAttachmentDescription depthAttachment = {
        .format = backend->depthFormat,
        .samples = pinfo.hasMsaa ? pass->ctx->samples : VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = pinfo.initialDepthLayout,
        .finalLayout = pinfo.finalDepthLayout
    };

    VkAttachmentDescription resolveAttachment = colorAttachment;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    u32 attachmentCount = 1;
    VkAttachmentDescription attachments[3] = {0};
    attachments[0] = colorAttachment;

    VkAttachmentReference colRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    
    VkAttachmentReference depthRef = {
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference resolveRef = {
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .colorAttachmentCount = 1,
        .pColorAttachments = &colRef
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    if(pinfo.hasDepth) {
        depthRef.attachment = attachmentCount;
        attachments[attachmentCount++] = depthAttachment;
        subpass.pDepthStencilAttachment = &depthRef;
        
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    if(pinfo.hasMsaa) {
        resolveRef.attachment = 0;
        attachments[0] = resolveAttachment;
        colRef.attachment = attachmentCount;
        attachments[attachmentCount++] = colorAttachment;
        subpass.pResolveAttachments = &resolveRef;
        dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if(pinfo.hasDepth)
            dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachmentCount,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VK_CHECK(vkCreateRenderPass(pass->ctx->device, &info, pass->ctx->allocator, &pass->handle));
}

void VulkanRenderPassDestroy(VulkanRenderPass* pass) {
    vkDestroyRenderPass(pass->ctx->device, pass->handle, pass->ctx->allocator);
    MF_SETMEM(pass, 0, sizeof(VulkanRenderPass));
}

void VulkanRenderPassBegin(VulkanRenderPass* pass, VulkanRenderPassBeginInfo info) {
    if(pass->begun)
        return;

    VkRenderPassBeginInfo bInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = info.clearValueCount,
        .pClearValues = info.clearValues,
        .framebuffer = info.fb->buffer,
        .renderArea = info.extent,
        .renderPass = pass->handle
    };

    vkCmdBeginRenderPass(info.cmdBuff, &bInfo, VK_SUBPASS_CONTENTS_INLINE);
    pass->begun = true;

    // Checking if attachments match
    {
        u32 attachmentCount = 1;
        if(pass->info.hasDepth)
            attachmentCount++;
        if(pass->info.hasMsaa)
            attachmentCount++;
        
        if(attachmentCount != info.fb->attachmentCount)
            return;
    }

    u32 idx = 0;
    // Color attachment
    info.fb->attachments[idx]->access = 0;
    info.fb->attachments[idx]->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    info.fb->attachments[idx]->layout = pass->info.initialLayout;

    // Depth attachment
    if(pass->info.hasDepth) {
        idx++;
        info.fb->attachments[idx]->access = 0;
        info.fb->attachments[idx]->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        info.fb->attachments[idx]->layout = pass->info.initialDepthLayout;
    }

    // Resolve attachment
    if(pass->info.hasMsaa) {
        idx++;
        info.fb->attachments[idx]->access = 0;
        info.fb->attachments[idx]->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        info.fb->attachments[idx]->layout = pass->info.initialLayout;
    }
}

void VulkanRenderPassEnd(VulkanRenderPass* pass, VkCommandBuffer cmdBuff, VulkanFramebuffer* fb) {
    if(!pass->begun)
        return;

    vkCmdEndRenderPass(cmdBuff);
    pass->begun = false;

    // Checking if attachments match
    {
        u32 attachmentCount = 1;
        if(pass->info.hasDepth)
            attachmentCount++;
        if(pass->info.hasMsaa)
            attachmentCount++;
        
        if(attachmentCount != fb->attachmentCount)
            return;
    }

    u32 idx = 0;
    // Color attachment
    fb->attachments[idx]->access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    fb->attachments[idx]->stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    fb->attachments[idx]->layout = pass->info.finalLayout;

    // Depth attachment
    if(pass->info.hasDepth) {
        idx++;
        fb->attachments[idx]->access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        fb->attachments[idx]->stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        fb->attachments[idx]->layout = pass->info.finalDepthLayout;
    }

    // Resolve attachment
    if(pass->info.hasMsaa) {
        idx++;
        fb->attachments[idx]->access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        fb->attachments[idx]->stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        fb->attachments[idx]->layout = pass->info.finalLayout;
    }
}

#ifdef __cplusplus
}
#endif