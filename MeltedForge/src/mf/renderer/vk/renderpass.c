#include "renderpass.h"

#include "common.h"

VkRenderPass VulkanRenderPassCreate(VulkanBackendCtx* ctx, VkFormat format, VkImageLayout initiaLay, VkImageLayout finalLay) { // TODO: Make the creation of renderpasses more genetic
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

    VkAttachmentDescription attachments[] = {
        colorAttachment
    };

    VkAttachmentReference colRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference refs[] = {
        colRef
    };

    VkSubpassDescription subpass = {
        .colorAttachmentCount = MF_ARRAYLEN(refs, VkAttachmentReference),
        .pColorAttachments = refs
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = MF_ARRAYLEN(attachments, VkAttachmentDescription),
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