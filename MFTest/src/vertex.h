#pragma once

#include <mf.h>

typedef struct Vertex_s {
    MFVec3 pos;
    MFVec3 normal;
    MFVec2 uv;
} Vertex;

MF_INLINE MFVertexInputBindingDescription getVertBindingDesc() {
    MFVertexInputBindingDescription desc;
    desc.binding = 0;
    desc.rate = MF_VERTEX_INPUT_RATE_VERTEX;
    desc.stride = sizeof(Vertex);

    return desc;
}

MF_INLINE MFVertexInputAttributeDescription* getVertAttribDescs(u32* count) {
    *count = 3;

    MFVertexInputAttributeDescription* desc = MF_ALLOCMEM(MFVertexInputAttributeDescription, sizeof(MFVertexInputAttributeDescription) * 3);
    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].offset = offsetof(Vertex, pos);
    desc[0].format = MF_FORMAT_R32G32B32_SFLOAT;
    
    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].offset = offsetof(Vertex, normal);
    desc[1].format = MF_FORMAT_R32G32B32_SFLOAT;

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].offset = offsetof(Vertex, uv);
    desc[2].format = MF_FORMAT_R32G32_SFLOAT;

    return desc;
}

MF_INLINE void vertBuilder(void* dst, MFModelVertexBuilderData data) {
    Vertex* vertex = (Vertex*)dst;

    vertex->pos = data.pos;
    vertex->normal = data.normal;
    vertex->uv = data.texCoord;
}