#include "fb.h"

#include "common.h"

VkFramebuffer VulkanFbCreate(VulkanBackendCtx* ctx, VkRenderPass pass, u32 imgViewCount, VkImageView* imgViews, VkExtent2D extent) {
    VkFramebufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .attachmentCount = imgViewCount,
        .pAttachments = imgViews,
        .layers = 1, // NOTE: Make it configurable
        .renderPass = pass,
        .width = extent.width,
        .height = extent.height
    };
    
    VkFramebuffer fb;
    VK_CHECK(vkCreateFramebuffer(ctx->device, &info, ctx->allocator, &fb));
    return fb;
}

void VulkanFbDestroy(VulkanBackendCtx* ctx, VkFramebuffer fb) {
    vkDestroyFramebuffer(ctx->device, fb, ctx->allocator);
}