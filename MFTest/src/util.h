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

MF_INLINE void SetUiStyle() {
    ImGuiIO* io = igGetIO_Nil();

    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;
    
    style->WindowRounding = 12.0f;
    style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    style->WindowPadding = (ImVec2){0.0f, 0.0f};
    style->FrameBorderSize = 3;
    style->FramePadding = (ImVec2){5, 5};
    style->FrameRounding = 6;
    style->TabRounding = 6;
    style->GrabRounding = 6;
    style->PopupRounding = 6;
    style->ChildRounding = 6;
    style->WindowRounding = 6;
    style->ScrollbarRounding = 6;
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.13f, 0.13f, 0.13f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.13f, 0.13f, 0.13f, 0.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.13f, 0.13f, 0.13f, 0.94f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.00f, 0.00f, 0.00f, 0.50f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.14f, 0.14f, 0.14f, 0.74f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.33f, 0.33f, 0.33f, 0.54f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.31f, 0.31f, 0.31f, 0.40f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.23f, 0.23f, 0.23f, 0.75f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.16f, 0.16f, 0.16f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.00f, 0.00f, 0.00f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.12f, 0.12f, 0.12f, 0.51f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.13f, 0.13f, 0.13f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.13f, 0.13f, 0.13f, 0.53f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.35f, 0.35f, 0.35f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.40f, 0.40f, 0.41f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.39f, 0.39f, 0.40f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.43f, 0.43f, 0.43f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.25f, 0.24f, 0.24f, 0.40f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.35f, 0.35f, 0.35f, 1.00f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.46f, 0.46f, 0.46f, 1.00f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.29f, 0.29f, 0.29f, 0.31f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.29f, 0.29f, 0.29f, 0.31f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.46f, 0.46f, 0.46f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.39f, 0.39f, 0.39f, 0.78f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.16f, 0.16f, 0.16f, 0.20f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.20f, 0.20f, 0.20f, 0.67f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.27f, 0.28f, 0.28f, 0.95f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.27f, 0.27f, 0.27f, 0.80f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.28f, 0.28f, 0.28f, 0.86f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.47f, 0.47f, 0.47f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.35f, 0.35f, 0.35f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.18f, 0.19f, 0.21f, 0.97f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.17f, 0.19f, 0.22f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline]  = (ImVec4){0.19f, 0.17f, 0.17f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.20f, 0.29f, 0.41f, 0.70f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f};
}
