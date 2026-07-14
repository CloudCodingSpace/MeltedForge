#ifdef __cplusplus
extern "C" {
#endif

#include "fb.h"

#include "common.h"

void VulkanFramebufferCreate(VulkanFramebuffer* fb, VulkanBackendCtx* ctx, VkRenderPass pass, u32 attachmentCount, VulkanImage** attachments, VkExtent2D extent) {
    MF_SETMEM(fb, 0, sizeof(VulkanFramebuffer));
    
    fb->ctx = ctx;
    fb->attachmentCount = attachmentCount;
    fb->pass = pass;
    fb->extent = extent;
    fb->attachments = MF_ALLOCMEM(VulkanImage*, sizeof(VulkanImage*) * attachmentCount);
    memcpy(fb->attachments, attachments, sizeof(VulkanImage*) * attachmentCount);

    VkImageView* views = MF_ALLOCMEM(VkImageView, sizeof(VkImageView) * attachmentCount);
    for(u32 i = 0; i < attachmentCount; i++) {
        views[i] = attachments[i]->view;
    }

    VkFramebufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .attachmentCount = attachmentCount,
        .pAttachments = views,
        .layers = 1, // TODO: Make it configurable if required
        .renderPass = pass,
        .width = extent.width,
        .height = extent.height
    };
    
    VK_CHECK(vkCreateFramebuffer(ctx->device, &info, ctx->allocator, &fb->buffer));
    MF_FREEMEM(views);
}

void VulkanFramebufferDestroy(VulkanFramebuffer* fb) {
    vkDestroyFramebuffer(fb->ctx->device, fb->buffer, fb->ctx->allocator);
    MF_FREEMEM(fb->attachments);
    MF_SETMEM(fb, 0, sizeof(VulkanFramebuffer));
}

#ifdef __cplusplus
}
#endif