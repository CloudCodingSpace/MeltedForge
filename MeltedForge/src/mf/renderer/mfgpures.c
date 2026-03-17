#include "mfgpures.h"

#include <vulkan/vulkan.h>

#include "vk/backend.h"

struct MFResourceSetLayout_s {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    MFRenderer* renderer;
};

struct MFResourceSet_s {
    VkDescriptorSet sets[FRAMES_IN_FLIGHT];
    MFResourceSetLayout* layout;
    MFRenderer* renderer;
};

void mfResourceSetLayoutCreate(MFResourceSetLayout* layout, u64 resDescLen, MFResourceDesc* resDescs, MFRenderer* renderer) {

}

void mfResourceSetLayoutDestroy(MFResourceSetLayout* layout) {
    
}

void mfResourceSetCreate(MFResourceSet* set, MFResourceSetLayout* layout, MFRenderer* renderer) {

}

void mfResourceSetDestroy(MFResourceSet* set) {

}

void mfResourceSetUpdate(MFResourceSet* set) {

}

void* mfGetResourceSetLayoutBackend(MFResourceSetLayout* layout) {

}

size_t mfGetResourceSetLayoutSizeInBytes(void) {
    return sizeof(MFResourceSetLayout);
}
size_t mfGetResourceSetSizeInBytes(void) {
    return sizeof(MFResourceSet);
}