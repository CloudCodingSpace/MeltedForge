#include "renderpass.h"

#include "common.h"

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VkFormat format, VkImageLayout initiaLay, VkImageLayout finalLay, b8 hasDepth, b8 renderTarget) { // TODO: Make the creation of renderpasses more generic
    VkAttachmentDescription colorAttachment = {
        .format = format,
        .initialLayout = initiaLay,
        .finalLayout = finalLay,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE
    };

    VkAttachmentDescription attachments[2] = {0};
    attachments[0] = colorAttachment;

    VkAttachmentReference colRef = {
        .attachment = 0,
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

    VkAttachmentDescription depthAttachment = {
        .format = ctx->depthFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    
    VkAttachmentReference depthRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    if(hasDepth) {        
        attachments[1] = depthAttachment;
        subpass.pDepthStencilAttachment = &depthRef;
        
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    if(renderTarget) {
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (hasDepth) ? 2 : 1,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkRenderPass pass;
    VK_CHECK(vkCreateRenderPass(ctx->device, &info, ctx->allocator, &pass));
    return pass;
}

void VulkanRenderPassDestroy(VulkanBackendCtx* ctx, VkRenderPass pass) {
    vkDestroyRenderPass(ctx->device, pass, ctx->allocator);
}