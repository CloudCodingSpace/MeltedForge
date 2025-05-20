#pragma once

#include <mf.h>

typedef struct Vertex_s {
    MFVec3 pos;
    MFVec3 color;
} Vertex;

MF_INLINE MFVertexInputBindingDescription getVertBindingDesc() {
    MFVertexInputBindingDescription desc;
    desc.binding = 0;
    desc.rate = MF_VERTEX_INPUT_RATE_VERTEX;
    desc.stride = sizeof(Vertex);

    return desc;
}

MF_INLINE MFVertexInputAttributeDescription* getVertAttribDescs(u32* count) {
    *count = 2;

    MFVertexInputAttributeDescription* desc = MF_ALLOCMEM(MFVertexInputAttributeDescription, sizeof(MFVertexInputAttributeDescription) * 2);
    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].offset = offsetof(Vertex, pos);
    desc[0].format = MF_FORMAT_R32G32B32_SFLOAT;
    
    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].offset = offsetof(Vertex, color);
    desc[1].format = MF_FORMAT_R32G32B32_SFLOAT;

    return desc;
}