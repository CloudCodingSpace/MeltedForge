#ifdef __cpluscplus
extern "C" {
#endif

#include "mfskybox.h"

#include "objects/mfmesh.h"

#include "mfpipeline.h"
#include "mfgpu_res.h"

#include "vk/backend.h"
#include "vk/pipeline.h"
#include "vk/image.h"
#include "vk/buffer.h"
#include "vk/fb.h"
#include "vk/renderpass.h"
#include "vk/command_buffer.h"

#include <stb/stb_image.h>

struct MFSkybox_s {
    MFMesh mesh;
    MFPipeline* pipeline;
    MFResourceSet* set;
    MFResourceSetLayout* layout;

    MFGpuImage* image;
    MFSkyboxConfig config;
    VulkanBackend* backend;
    MFRenderer* renderer;
    bool init;
};

static void convertEnvMapToSkybox(MFSkybox* skybox, MFSkyboxConfig config, MFRenderer* renderer);

MFSkybox* mfSkyboxCreate(MFSkyboxConfig config, MFRenderer* renderer) {
    MF_PANIC_IF(config.hdrEnvironmentPath == mfnull, mfGetLogger(), "The hdr environment map path shouldn't be null!");
    MF_PANIC_IF(config.faceSize == 0, mfGetLogger(), "The face size of the skybox provided shouldn't be null!");

    MFSkybox* skybox = MF_ALLOCMEM(MFSkybox, sizeof(MFSkybox));

    skybox->backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    skybox->config = config;
    skybox->config.hdrEnvironmentPath = mfStringDuplicate(config.hdrEnvironmentPath);
    skybox->renderer = renderer;

    MFGpuImageConfig info = {
        .binding = config.binding,
        .generateMipmaps = false, // TODO: Later enable mipmaps for skybox
        .width = config.faceSize,
        .height = config.faceSize,
        .isCubemap = true,
        .imageFormat = MF_FORMAT_R32G32B32A32_SFLOAT,
        .binding = 0
    };
    skybox->image = mfGpuImageCreate(renderer, info);

    // Skybox
    {
        f32 vertices[] = {
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f
        };

        u32 indices[] = {
            0, 2, 1,
            0, 3, 2,

            4, 3, 0,
            4, 7, 3,

            1, 2, 6,
            1, 6, 5,

            5, 6, 7,
            5, 7, 4,

            3, 7, 6,
            3, 6, 2,

            4, 0, 1,
            4, 1, 5
        };

        mfMeshCreate(&skybox->mesh, renderer, sizeof(f32) * MF_ARRAYLEN(vertices, f32), vertices, MF_ARRAYLEN(indices, u32), indices);
    }
    // Resources
    {
        MFResourceDescription description = mfGpuImageGetDescription(skybox->image);
        skybox->layout = mfResourceSetLayoutCreate(1, &description, 1, renderer);

        skybox->set = mfResourceSetCreate(skybox->layout, renderer);

        {
            MFArray array = mfArrayCreate(mfGetLogger(), 1, sizeof(MFGpuImage*));
            mfArrayAddElement(array, MFGpuImage*, mfGetLogger(), skybox->image);
            mfResourceSetUpdate(skybox->set, &array, mfnull);
            mfArrayDestroy(&array, mfGetLogger());
        }
    }
    // Pipeline
    {
        MFVertexInputBindingDescription binding = {
            .binding = 0,
            .rate = MF_VERTEX_INPUT_RATE_VERTEX,
            .stride = sizeof(f32) * 3
        };

        MFVertexInputAttributeDescription attribute = {
            .binding = 0,
            .format = MF_FORMAT_R32G32B32_SFLOAT,
            .location = 0,
            .offset = 0
        };

        MFPushConstantRange range = {
            .offset = 0,
            .size = sizeof(MFMat4) * 2,
            .stage = MF_SHADER_STAGE_VERTEX
        };

        MFPipelineConfig info = {
            .hasDepth = true,
            .extent = (MFVec2){100.0f, 100.0f}, // NOTE: Leaving at 100,100 since pipeline supports dynamic rendering, that time this exten won't matter.
            .vertPath = "mfassets/shaders/mfskybox.vert.spv",
            .fragPath = "mfassets/shaders/mfskybox.frag.spv",
            .attributesCount = 1,
            .attributes = &attribute,
            .bindingsCount = 1,
            .bindings = &binding,
            .pushConstRangeCount = 1,
            .pushConstRanges = &range,
            .depthCompareOp = MF_COMPARE_OP_LESS_OR_EQUAL,
            .resourceLayoutCount = 1,
            .resourceLayouts = &skybox->layout,
            .cullMode = MF_CULL_MODE_NONE
        };

        skybox->pipeline = mfPipelineCreate(renderer, &info);
    }

    convertEnvMapToSkybox(skybox, config, renderer);

    skybox->init = true;
    return skybox;
}

void mfSkyboxDestroy(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");
    
    mfMeshDestroy(&skybox->mesh);
    mfPipelineDestroy(skybox->pipeline);
    mfResourceSetDestroy(skybox->set);
    mfResourceSetLayoutDestroy(skybox->layout);
    mfGpuImageDestroy(skybox->image);
    
    MF_FREEMEM(skybox->config.hdrEnvironmentPath);
    MF_SETMEM(skybox, 0, sizeof(MFSkybox));
    MF_FREEMEM(skybox);
}

size_t mfSkyboxGetSizeInBytes(void) {
    return sizeof(MFSkybox);
}

void mfSkyboxRender(MFSkybox* skybox, MFMat4 projection, MFMat4 view) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");

    mfPipelineBind(skybox->pipeline, mfRendererGetViewport(skybox->renderer), mfRendererGetScissor(skybox->renderer));
    
    MFMat4 data[] = {
        projection,
        view
    };

    mfResourceSetBind(skybox->set, skybox->pipeline);
    mfPipelinePushConstant(skybox->pipeline, MF_SHADER_STAGE_VERTEX, 0, sizeof(MFMat4) * 2, data);
    mfMeshRender(&skybox->mesh);
}

MFGpuImage* mfSkyboxGetCubemapImage(MFSkybox* skybox) {
    MF_PANIC_IF(skybox == mfnull, mfGetLogger(), "The skybox handle provided shouldn't be null!");
    MF_PANIC_IF(!skybox->init, mfGetLogger(), "The skybox handle provided should be initialised!");

    return skybox->image;
}

static void convertEnvMapToSkybox(MFSkybox* skybox, MFSkyboxConfig config, MFRenderer* renderer) {
    VulkanBackendCtx* ctx = &skybox->backend->ctx;
    MFGpuImage* image = mfnull;
    MFResourceSet* set = mfnull;
    MFResourceSetLayout* layout = mfnull; 
    VulkanImage depthImage, tempImage;
    VulkanPipeline pipeline;
    VkRenderPass pass = mfnull;
    VkImageView view;
    VkFramebuffer fb;
    VkCommandBuffer cmdBuff = mfnull;
    VkFence fence = mfnull;
    VulkanImage* cubemapImage = (VulkanImage*)mfGpuImageGetBackend(skybox->image);

    // Renderpass
    {
        VulkanRenderPassInfo info = {
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .format = MF_FORMAT_R32G32B32A32_SFLOAT,
            .hasDepth = true
        };
        pass = VulkanRenderPassCreate(ctx, info);
    }
    // 2D env map
    {
        stbi_set_flip_vertically_on_load(true);
        i32 width, height, channels;
        float* data = stbi_loadf(config.hdrEnvironmentPath, &width, &height, &channels, 4);
        if(data == mfnull) {
            MF_FATAL_ABORT(mfGetLogger(), "Failed to load hdr image. More reasons by image loader :- %s", stbi_failure_reason());
        }

        MFGpuImageConfig info = {
            .width = width,
            .height = height,
            .generateMipmaps = false,
            .imageFormat = MF_FORMAT_R32G32B32A32_SFLOAT,
            .isCubemap = false,
            .pixels = data,
            .binding = 0
        };

        image = mfGpuImageCreate(renderer, info);

        stbi_image_free(data);
    }
    // Depth & temp image
    {
        VulkanImageInfo info = {
            .ctx = ctx,
            .width = config.faceSize,
            .height = config.faceSize,
            .gpuResource = false,
            .pixels = mfnull,
            .format = ctx->depthFormat,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .arrayLayers = 1,
            .type = VK_IMAGE_TYPE_2D
        };

        VulkanImageCreate(&depthImage, info);

        info.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        info.format = MF_FORMAT_R32G32B32A32_SFLOAT;
        info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        VulkanImageCreate(&tempImage, info);
    }
    // Resources sets and layouts
    {
        MFResourceDescription description = mfGpuImageGetDescription(image);
        layout = mfResourceSetLayoutCreate(1, &description, 1, renderer);

        set = mfResourceSetCreate(layout, renderer);

        MFArray array = mfArrayCreate(mfGetLogger(), 1, sizeof(MFGpuImage*));
        mfArrayAddElement(array, MFGpuImage*, mfGetLogger(), image);
        mfResourceSetUpdate(set, &array, mfnull);
        mfArrayDestroy(&array, mfGetLogger());
    }
    // Pipeline
    {
        VkVertexInputBindingDescription binding = {
            .binding = 0,
            .inputRate = MF_VERTEX_INPUT_RATE_VERTEX,
            .stride = sizeof(f32) * 3
        };

        VkVertexInputAttributeDescription attribute = {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .location = 0,
            .offset = 0
        };

        VkPushConstantRange range = {
            .offset = 0,
            .size = sizeof(MFMat4) * 2,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        };

        VkDescriptorSetLayout lay = (VkDescriptorSetLayout)mfResourceSetLayoutGetBackend(layout);

        VulkanPipelineInfo info = {
            .attributesCount = 1,
            .attributes = &attribute,
            .bindingsCount = 1,
            .bindings = &binding,
            .pushConstRangesCount = 1,
            .pushConstRanges = &range,
            .vertPath = "mfassets/shaders/mfskyboxConvert.vert.spv",
            .fragPath = "mfassets/shaders/mfskyboxConvert.frag.spv",
            .cache = skybox->backend->pipelineCache,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .renderpass = pass,
            .extent = (VkExtent2D){ .width = config.faceSize, .height = config.faceSize },
            .hasDepth = true,
            .setLayoutCount = 1,
            .setLayouts = &lay,
            .cullMode = VK_CULL_MODE_NONE
        };
        VulkanPipelineCreate(ctx, &pipeline, &info);
    }
    // Command buffer
    cmdBuff = VulkanCommandBufferAllocate(ctx, ctx->commandPool, true);
    // Image views and framebuffers
    {
        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .components = (VkComponentMapping){
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .format = MF_FORMAT_R32G32B32A32_SFLOAT,
            .image = tempImage.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseArrayLayer = 0,
                .baseMipLevel = 0,
                .levelCount = 1,
                .layerCount = 1
            }
        };
        VK_CHECK(vkCreateImageView(ctx->device, &viewInfo, ctx->allocator, &view));

        VkImageView attachments[2] = {
            view,
            depthImage.view
        };
        VkFramebufferCreateInfo fbInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = config.faceSize,
            .height = config.faceSize,
            .layers = 1,
            .renderPass = pass
        };
        VK_CHECK(vkCreateFramebuffer(ctx->device, &fbInfo, ctx->allocator, &fb));
    }
    // Fence
    {
        VkFenceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };
        VK_CHECK(vkCreateFence(ctx->device, &info, ctx->allocator, &fence));
    }

    // Main recording
    {
        VulkanCommandBufferBegin(cmdBuff);

        // Transitioning cubemap to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        {
            VkImageMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .image = cubemapImage->image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseArrayLayer = 0,
                    .layerCount = 6,
                    .baseMipLevel = 0,
                    .levelCount = 1
                }
            };

            vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
        }

        MFMat4 proj = mfMat4Perspective(90.0f * MF_DEG2RAD_MULTIPLIER, 1.0f, 0.1f, 100.0f);
        
        MFMat4 views[6] = {
            mfMat4LookAt((MFVec3){0,0,0}, (MFVec3){1,0,0}, (MFVec3){0,-1,0}),
            mfMat4LookAt((MFVec3){0,0,0}, (MFVec3){-1,0,0}, (MFVec3){0,-1,0}),
            mfMat4LookAt((MFVec3){0,0,0}, (MFVec3){0,1,0}, (MFVec3){0,0,1}),
            mfMat4LookAt((MFVec3){0,0,0}, (MFVec3){0,-1,0}, (MFVec3){0,0,-1}),
            mfMat4LookAt((MFVec3){0,0,0}, (MFVec3){0,0,1}, (MFVec3){0,-1,0}),
            mfMat4LookAt((MFVec3){0,0,0}, (MFVec3){0,0,-1}, (MFVec3){0,-1,0})
        };

        VkClearValue clearValue[2] = {
            skybox->backend->clearColor
        };
        clearValue[1].depthStencil.depth = 1.0f;
        clearValue[1].depthStencil.stencil = 0.0f;

        for(u32 i = 0; i < 6; i++) {
            VkRenderPassBeginInfo begin = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .clearValueCount = 2,
                .pClearValues = clearValue,
                .framebuffer = fb,
                .renderArea = {
                    .extent = { .width = config.faceSize, .height = config.faceSize },
                    .offset = {0, 0}
                },
                .renderPass = pass
            };
            vkCmdBeginRenderPass(cmdBuff, &begin, VK_SUBPASS_CONTENTS_INLINE);
            
            VkViewport vp = {
                .x = 0,
                .y = 0,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
                .width = config.faceSize,
                .height = config.faceSize
            };

            VkRect2D rect = {
                .extent = { .width = config.faceSize, .height = config.faceSize },
                .offset = { 0, 0 }
            };

            VulkanPipelineBind(&pipeline, vp, rect, cmdBuff);

            VkDescriptorSet* sets = (VkDescriptorSet*)mfResourceSetGetBackend(set);
            vkCmdBindDescriptorSets(cmdBuff, pipeline.bindPoint, pipeline.layout, 0, 1, &sets[0], 0, mfnull);
            
            MFMat4 pcData[] = {
                proj,
                views[i]
            };
            vkCmdPushConstants(cmdBuff, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MFMat4) * 2, pcData);
            VulkanBuffer* vertBuffer = (VulkanBuffer*)mfGpuBufferGetBackend(skybox->mesh.vertBuffer);
            VulkanBuffer* indexBuffer = (VulkanBuffer*)mfGpuBufferGetBackend(skybox->mesh.indBuffer);
            VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(cmdBuff, 0, 1, &vertBuffer[0].handle, offsets);
            vkCmdBindIndexBuffer(cmdBuff, indexBuffer[0].handle, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmdBuff, skybox->mesh.vertCount, 1, 0, 0, 0);

            vkCmdEndRenderPass(cmdBuff);

            // Transition temp image to transfer src
            {
                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .image = tempImage.image,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                        .baseMipLevel = 0,
                        .levelCount = 1
                    }
                };

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
            }
            // Copying to cubemap
            {
                VkImageCopy copy = {
                    .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                        .mipLevel = 0
                    },
                    .dstSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseArrayLayer = i,
                        .layerCount = 1,
                        .mipLevel = 0
                    },
                    .extent = {
                        config.faceSize,
                        config.faceSize,
                        1
                    }
                };

                vkCmdCopyImage(cmdBuff, tempImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubemapImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            }
        }

        // Transition cubemap layer to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        {
            VkImageMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = cubemapImage->image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 6
                },
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
            };

            vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, mfnull, 0, mfnull, 1, &barrier);
        }

        VulkanCommandBufferEnd(cmdBuff);

        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmdBuff
        };
        VK_CHECK(vkQueueSubmit(ctx->queueData.graphicsQueue, 1, &info, fence));
    }

    // Cleaning up
    {
        VK_CHECK(vkWaitForFences(ctx->device, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(ctx->device, fence, ctx->allocator);
        vkDestroyFramebuffer(ctx->device, fb, ctx->allocator);
        vkDestroyImageView(ctx->device, view, ctx->allocator);

        VulkanCommandBufferFree(ctx, cmdBuff, ctx->commandPool);
        VulkanImageDestroy(&tempImage);
        VulkanImageDestroy(&depthImage);
        VulkanPipelineDestroy(ctx, &pipeline);
        VulkanRenderPassDestroy(ctx, pass);
        mfResourceSetDestroy(set);
        mfResourceSetLayoutDestroy(layout);
        mfGpuImageDestroy(image);
    }
}

#ifdef __cpluscplus
}
#endif