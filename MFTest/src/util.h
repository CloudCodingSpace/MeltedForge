#pragma once

#include <mf.h>

typedef struct Vertex_s {
    MFVec3 pos;
    MFVec3 normal;
    MFVec3 tangent;
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
    *count = 4;

    MFVertexInputAttributeDescription* desc = MF_ALLOCMEM(MFVertexInputAttributeDescription, sizeof(MFVertexInputAttributeDescription) * (*count));
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
    
    desc[3].binding = 0;
    desc[3].location = 3;
    desc[3].offset = offsetof(Vertex, tangent);
    desc[3].format = MF_FORMAT_R32G32B32_SFLOAT;

    return desc;
}

MF_INLINE void vertBuilder(void* dst, MFModelVertexBuilderData data) {
    Vertex* vertex = (Vertex*)dst;

    vertex->pos = data.pos;
    vertex->normal = data.normal;
    vertex->uv = data.texCoord;
    vertex->tangent = data.tangent;
}

MF_INLINE void SetUiStyle() {
    ImGuiIO* io = igGetIO_Nil();

    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    style->WindowPadding = (ImVec2){0.0f, 0.0f};
    style->FramePadding = (ImVec2){8.0f, 5.0f};
    style->ItemSpacing = (ImVec2){10.0f, 6.0f};
    style->ItemInnerSpacing = (ImVec2){9.0f, 5.0f};
    style->ScrollbarSize = 14.0f;
    style->GrabMinSize = 10.0f;
    style->WindowBorderSize = 1.0f;
    style->FrameBorderSize = 1.0f;
    style->TabBorderSize = 0.0f;
    style->WindowRounding = 7.0f;
    style->ChildRounding = 7.0f;
    style->FrameRounding = 6.0f;
    style->PopupRounding = 6.0f;
    style->ScrollbarRounding = 7.0f;
    style->GrabRounding = 5.0f;
    style->TabRounding = 6.0f;

    colors[ImGuiCol_Text] =                     (ImVec4){0.863f, 0.933f, 0.961f, 1.000f};
    colors[ImGuiCol_TextDisabled] =             (ImVec4){0.478f, 0.576f, 0.631f, 1.000f};
    colors[ImGuiCol_WindowBg] =                 (ImVec4){0.024f, 0.063f, 0.102f, 1.000f};
    colors[ImGuiCol_ChildBg] =                  (ImVec4){0.043f, 0.102f, 0.157f, 0.900f};
    colors[ImGuiCol_PopupBg] =                  (ImVec4){0.024f, 0.063f, 0.102f, 0.980f};
    colors[ImGuiCol_Border] =                   (ImVec4){0.090f, 0.184f, 0.259f, 1.000f};
    colors[ImGuiCol_FrameBg] =                  (ImVec4){0.043f, 0.102f, 0.157f, 1.000f};
    colors[ImGuiCol_FrameBgHovered] =           (ImVec4){0.075f, 0.647f, 0.784f, 0.220f};
    colors[ImGuiCol_FrameBgActive] =            (ImVec4){0.075f, 0.647f, 0.784f, 0.350f};
    colors[ImGuiCol_TitleBg] =                  (ImVec4){0.043f, 0.102f, 0.157f, 1.000f};
    colors[ImGuiCol_TitleBgActive] =            (ImVec4){0.090f, 0.184f, 0.259f, 1.000f};
    colors[ImGuiCol_MenuBarBg] =                (ImVec4){0.043f, 0.102f, 0.157f, 1.000f};
    colors[ImGuiCol_ScrollbarBg] =              (ImVec4){0.024f, 0.063f, 0.102f, 0.800f};
    colors[ImGuiCol_ScrollbarGrab] =            (ImVec4){0.090f, 0.184f, 0.259f, 0.900f};
    colors[ImGuiCol_ScrollbarGrabHovered] =     (ImVec4){0.075f, 0.647f, 0.784f, 0.650f};
    colors[ImGuiCol_ScrollbarGrabActive] =      (ImVec4){0.075f, 0.647f, 0.784f, 0.850f};
    colors[ImGuiCol_CheckMark] =                (ImVec4){0.075f, 0.647f, 0.784f, 1.000f};
    colors[ImGuiCol_SliderGrab] =               (ImVec4){0.075f, 0.647f, 0.784f, 0.850f};
    colors[ImGuiCol_SliderGrabActive] =         (ImVec4){0.075f, 0.647f, 0.784f, 1.000f};
    colors[ImGuiCol_Button] =                   (ImVec4){0.075f, 0.647f, 0.784f, 0.230f};
    colors[ImGuiCol_ButtonHovered] =            (ImVec4){0.075f, 0.647f, 0.784f, 0.420f};
    colors[ImGuiCol_ButtonActive] =             (ImVec4){0.075f, 0.647f, 0.784f, 0.650f};
    colors[ImGuiCol_Header] =                   (ImVec4){0.075f, 0.647f, 0.784f, 0.220f};
    colors[ImGuiCol_HeaderHovered] =            (ImVec4){0.075f, 0.647f, 0.784f, 0.420f};
    colors[ImGuiCol_HeaderActive] =             (ImVec4){0.075f, 0.647f, 0.784f, 0.650f};
    colors[ImGuiCol_Separator] =                (ImVec4){0.090f, 0.184f, 0.259f, 1.000f};
    colors[ImGuiCol_SeparatorHovered] =         (ImVec4){0.075f, 0.647f, 0.784f, 0.550f};
    colors[ImGuiCol_SeparatorActive] =          (ImVec4){0.075f, 0.647f, 0.784f, 0.750f};
    colors[ImGuiCol_ResizeGrip] =               (ImVec4){0.075f, 0.647f, 0.784f, 0.260f};
    colors[ImGuiCol_ResizeGripHovered] =        (ImVec4){0.075f, 0.647f, 0.784f, 0.500f};
    colors[ImGuiCol_ResizeGripActive] =         (ImVec4){0.075f, 0.647f, 0.784f, 0.700f};
    colors[ImGuiCol_Tab] =                      (ImVec4){0.043f, 0.102f, 0.157f, 1.000f};
    colors[ImGuiCol_TabHovered] =               (ImVec4){0.075f, 0.647f, 0.784f, 0.320f};
    colors[ImGuiCol_TabSelected] =                (ImVec4){0.075f, 0.647f, 0.784f, 0.240f};
    colors[ImGuiCol_TabHovered] =             (ImVec4){0.043f, 0.102f, 0.157f, 1.000f};
    colors[ImGuiCol_DockingPreview] =           (ImVec4){0.075f, 0.647f, 0.784f, 0.320f};
    colors[ImGuiCol_DockingEmptyBg] =           (ImVec4){0.024f, 0.063f, 0.102f, 1.000f};
    colors[ImGuiCol_TableHeaderBg] =            (ImVec4){0.043f, 0.102f, 0.157f, 1.000f};
    colors[ImGuiCol_TableBorderStrong] =        (ImVec4){0.090f, 0.184f, 0.259f, 1.000f};
    colors[ImGuiCol_TableBorderLight] =         (ImVec4){0.090f, 0.184f, 0.259f, 0.450f};
    colors[ImGuiCol_TableRowBgAlt] =            (ImVec4){0.043f, 0.102f, 0.157f, 0.450f};
    colors[ImGuiCol_TextSelectedBg] =           (ImVec4){0.075f, 0.647f, 0.784f, 0.270f};
}
