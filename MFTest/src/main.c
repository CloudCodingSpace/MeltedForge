#include <mf.h>

#include <stb/stb_image.h>

#include "vertex.h"

typedef struct MFTState_s {
    MFPipeline* pipeline;
    MFGpuBuffer** ubos;
    MFGpuImage* tex;
    MFModel model;
    MFCamera camera;
} MFTState;

typedef struct UBOData_s {
    MFMat4 proj;
    MFMat4 view;
    MFMat4 model;
} UBOData;

static void MFTOnInit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest init\n");

    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));
    
    MFTState* state = (MFTState*)pstate;
    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);
    
    mfCameraCreate(&state->camera, appState->window, 60, 0.01f, 1000.0f, 0.025f, 0.075f, (MFVec3){0.0f, 0.0f, 2.0f});

    // UBO & Model
    {
        state->ubos = MF_ALLOCMEM(MFGpuBuffer*, sizeof(MFGpuBuffer*) * mfGetRendererFramesInFlight());
        for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
            state->ubos[i] = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        }

        mfModelLoadAndCreate(&state->model, "meshes/BossDragon.obj", appState->renderer, sizeof(Vertex), vertBuilder);

        MFGpuBufferConfig config = {
            .type = MF_GPU_BUFFER_TYPE_UBO,
            .size = sizeof(UBOData),
            .binding = 3,
            .stage = MF_SHADER_STAGE_VERTEX
        };

        UBOData uboData = {
            .proj = state->camera.proj,
            .view = state->camera.view,
            .model = mfMat4Identity()
        };

        for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
            mfGpuBufferAllocate(state->ubos[i], config, appState->renderer);
            mfGpuBufferUploadData(state->ubos[i], &uboData);
        }
    }
    // Image
    {
        u32 width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        u8* pixels = stbi_load("meshes/BossDragon.png", &width, &height, &channels, 4);
        if (!pixels) {
            slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- \n");
            slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, stbi_failure_reason());
            slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "\n");
            return;
        }

        state->tex = MF_ALLOCMEM(MFGpuImage, mfGetGpuImageSizeInBytes());
        
        MFGpuImageConfig config = {
            .width = width,
            .height = height,
            .pixels = pixels,
            .binding = 0
        };
        mfGpuImageCreate(state->tex, appState->renderer, config);

        stbi_image_free(pixels);
    }
    // Pipeline
    {
        u32 attribCount = 0, bindingCount = 1;
        MFVertexInputAttributeDescription* attribDescs = getVertAttribDescs(&attribCount);
        MFVertexInputBindingDescription bindingDesc = getVertBindingDesc();

        u32 imageCount = 1;
        MFGpuImage* images[] = {
            state->tex
        };

        MFPipelineConfig info = {
            .extent = (MFVec2){ .x = winConfig->width, .y = winConfig->height },
            .hasDepth = true,
            .transparent = false,
            .vertPath = "shaders/default.vert.spv",
            .fragPath = "shaders/default.frag.spv",
            .attribDescsCount = attribCount,
            .attribDescs = attribDescs,
            .bindingDescsCount = bindingCount,
            .bindingDescs = &bindingDesc,
            .imgCount = imageCount,
            .images = images,
            .buffCount = mfGetRendererFramesInFlight(),
            .buffers = state->ubos
        };
        mfPipelineInit(state->pipeline, appState->renderer, &info);

        MF_FREEMEM(attribDescs);
    }
    
    // UI customization
    {
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
}

static void MFTOnDeinit(void* pstate, void* pappState) {
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "MFTest deinit\n");
    MFTState* state = (MFTState*)pstate;
    
    mfGpuImageDestroy(state->tex);
    
    for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
        mfGpuBufferFree(state->ubos[i]);
        MF_FREEMEM(state->ubos[i]);
    }

    mfCameraDestroy(&state->camera);
    mfModelDestroy(&state->model);
    mfPipelineDestroy(state->pipeline);
    
    MF_FREEMEM(state->ubos);
    MF_FREEMEM(state->pipeline);
    MF_FREEMEM(state->tex);
}

static void MFTOnRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);
    
    mfPipelineBind(state->pipeline, mfRendererGetViewport(winConfig), mfRendererGetScissor(winConfig));
    mfModelRender(&state->model);
}

static void MFTOnUIRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    
    igBegin("MFTest", mfnull, ImGuiWindowFlags_None);

    igText("FPS :- %.3f", igGetIO_Nil()->Framerate);

    igEnd();
}

static void MFTOnUpdate(void* pstate, void* pappState) {
    MFDefaultAppState* aState = (MFDefaultAppState*)pappState;
    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* winConfig = mfGetWindowConfig(aState->window);

    printf("Rendering took :- %fms\n", mfGetRendererGetFrameTime(aState->renderer));

    state->camera.update(&state->camera, mfGetRendererGetDeltaTime(aState->renderer), mfnull);

    UBOData uboData = {
        .proj = state->camera.proj,
        .view = state->camera.view,
        // .model = mfMat4Mul(mfMat4RotateX(mfGetCurrentTime() * 90.0f * MF_DEG2RAD_MULTIPLIER), mfMat4RotateZ(mfGetCurrentTime() * 90.0f * MF_DEG2RAD_MULTIPLIER))
        .model = mfMat4Identity()
    };

    mfGpuBufferUploadData(state->ubos[mfGetRendererCurrentFrameIdx(aState->renderer)], &uboData);

    if(mfInputIsKeyPressed(aState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(aState->window);
    }
}

MFAppConfig mfClientCreateAppConfig() {
    MFAppConfig config = mfCreateDefaultApp("MFTest");
    
    config.layerCount = 1;
    config.layers = MF_ALLOCMEM(MFLayer, sizeof(MFLayer) * config.layerCount);
    config.layers[0] = (MFLayer){
        .state = MF_ALLOCMEM(MFTState, sizeof(MFTState)),
        .onInit = &MFTOnInit,
        .onDeinit = &MFTOnDeinit,
        .onRender = &MFTOnRender,
        .onUpdate = &MFTOnUpdate,
        .onUIRender = &MFTOnUIRender
    };
    config.winConfig.resizable = true;
    config.vsync = false;
    config.enableUI = true;

    return config;
}