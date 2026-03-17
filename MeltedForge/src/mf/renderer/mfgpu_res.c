#include "mfgpu_res.h"

#include <vulkan/vulkan.h>

#include "vk/backend.h"

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    MFRenderer* renderer;
    MFArray resDescs;
    u64 imageCount, bufferCount;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
};

void mfResourceSetLayoutCreate(MFResourceSetLayout* layout, u64 resDescLen, MFResourceDesc* resDescs, MFRenderer* renderer) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The provided renderer handle shouldn't be null!");
    MF_PANIC_IF(resDescLen == 0, mfGetLogger(), "The provided resource description count shouldn't be 0!");
    MF_PANIC_IF(resDescs == mfnull, mfGetLogger(), "The provided resource descriptions shouldn't be null!");
    
    layout->renderer = renderer;
    layout->resDescs = mfArrayCreate(mfGetLogger(), resDescLen, sizeof(MFResourceDesc));
    layout->resDescs.len = resDescLen;

    for(u64 i = 0; i < resDescLen; i++) {
        mfArrayGet(layout->resDescs, MFResourceDesc, i) = resDescs[i];
    }

    // TODO: Support more shader res type!
    u64 imageCount = 0, bufferCount = 0;
    for(u64 i = 0; i < resDescLen; i++) {
        if(resDescs[i].descriptorType == MF_RES_DESCRIPTION_TYPE_COMBINED_IMAGE_SAMPLER) {
            imageCount++;
        }
        if(resDescs[i].descriptorType == MF_RES_DESCRIPTION_TYPE_UNIFORM_BUFFER) {
            bufferCount++;;
        }
    }

    layout->imageCount = imageCount;
    layout->bufferCount = bufferCount;

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    // Descriptor pool
    {
        u32 count = 0;
        VkDescriptorPoolSize sizes[2] = {0};

        if(imageCount != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * FRAMES_IN_FLIGHT };
        }
        if(imageCount != 0) {
            sizes[count++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferCount * FRAMES_IN_FLIGHT };
        }

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1000 * FRAMES_IN_FLIGHT, //! FIXME: INSTEAD OF ASSUMING 1000 * FRAMES_IN_FLIGHT, CREATE A DYNAMIC DESCRIPTOR POOL ALLOCATOR
            .poolSizeCount = count,
            .pPoolSizes = sizes,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        };

        VK_CHECK(vkCreateDescriptorPool(ctx->device, &info, ctx->allocator, &layout->pool));
    }

    // Descriptor layout
    {
        VkDescriptorSetLayoutBinding* layBindings = MF_ALLOCMEM(VkDescriptorSetLayoutBinding, sizeof(VkDescriptorSetLayoutBinding) * resDescLen);
        u32 uniqueBindingCount = 0;

        for(u32 i = 0; i < resDescLen; i++) {
            bool bindingExists = false;
            for(u32 j = 0; j < uniqueBindingCount; j++) {
                if(layBindings[j].binding == resDescs[i].binding) {
                    bindingExists = true;
                    break;
                }
            }

            if(!bindingExists) {
                layBindings[uniqueBindingCount].binding = resDescs[i].binding;
                layBindings[uniqueBindingCount].descriptorType = (VkDescriptorType)((int)resDescs[i].descriptorType);
                layBindings[uniqueBindingCount].descriptorCount = resDescs[i].descriptorCount;
                layBindings[uniqueBindingCount].stageFlags = (VkShaderStageFlags)((int)resDescs[i].stageFlags);
                uniqueBindingCount++;
            }
        }

        MF_DO_IF(uniqueBindingCount != resDescLen, {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The bindings of each resource description in a layout must be unique! But the provided descriptions aren't unique!");
        });

        VkDescriptorSetLayoutCreateInfo layInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = uniqueBindingCount,
            .pBindings = layBindings
        };

        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device, &layInfo, ctx->allocator, &layout->layout));
        MF_FREEMEM(layBindings);
    }
}

void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(layout->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    vkDestroyDescriptorPool(ctx->device, layout->pool, ctx->allocator);
    vkDestroyDescriptorSetLayout(ctx->device, layout->layout, ctx->allocator);

    mfArrayDestroy(&layout->resDescs, mfGetLogger());

    MF_SETMEM(layout, 0, sizeof(MFResourceSetLayout));
}

void mfResourceSetCreate(MFResourceSet* set, MFResourceSetLayout* layout, MFRenderer* renderer) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The resource set layout handle provided shoudln't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    set->layout = layout;
    set->renderer = renderer;

    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = layout->pool,
        .descriptorSetCount = FRAMES_IN_FLIGHT,
        .pSetLayouts = &layout->layout
    };

    VK_CHECK(vkAllocateDescriptorSets(ctx->device, &info, set->sets));
}

void mfResourceSetDestroy(MFResourceSet* set) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    
    VulkanBackend* backend = (VulkanBackend*)mfRendererGetBackend(set->renderer);
    VulkanBackendCtx* ctx = &backend->ctx;

    VK_CHECK(vkFreeDescriptorSets(ctx->device, set->layout->pool, FRAMES_IN_FLIGHT, set->sets));

    MF_SETMEM(set, 0, sizeof(MFResourceSet));
}

void mfResourceSetUpdate(MFResourceSet* set, MFArray* images, MFArray* buffers) {
    MF_PANIC_IF(set == mfnull, mfGetLogger(), "The resource set handle provided shouldn't be null!");
    MF_PANIC_IF(images == mfnull, mfGetLogger(), "The images array provided shouldn't be null!");
    MF_PANIC_IF(buffers == mfnull, mfGetLogger(), "The buffer array provided shouldn't be null!");
    MF_PANIC_IF(images->len != set->layout->imageCount, mfGetLogger(), "The image array doesn't follow the resource set layout!");
    MF_PANIC_IF(buffers->len != set->layout->bufferCount, mfGetLogger(), "The buffer array doesn't follow the resource set layout!");

    // TODO: Implement this function
}

void* mfGetResourceSetLayoutBackend(MFResourceSetLayout* layout) {
    MF_PANIC_IF(layout == mfnull, mfGetLogger(), "The provided resource set layout shouldn't be null!");
    
    return layout->layout;
}

size_t mfGetResourceSetLayoutSizeInBytes(void) {
    return sizeof(MFResourceSetLayout);
}
size_t mfGetResourceSetSizeInBytes(void) {
    return sizeof(MFResourceSet);
}