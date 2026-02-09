#include "mftest.h"
#include "cimgui.h"
#include "core/mfmaths.h"
#include "ecs/mfscene.h"
#include "renderer/mfrenderer.h"
#include "slog/slog.h"
#include "util.h"

#define INFO(logger, msg, ...) slogLogConsole(logger, SLOG_SEVERITY_INFO, msg, ##__VA_ARGS__)

#pragma region PipelineFuncs

static void CreatePipeline(MFTState* state) {
    u32 attribCount = 0, bindingCount = 1;
    MFVertexInputAttributeDescription* attribDescs = getVertAttribDescs(&attribCount);
    MFVertexInputBindingDescription bindingDesc = getVertBindingDesc();

    u32 imageCount = 1;
    MFGpuImage* images[] = {
        mfArrayGet(state->modelMatImgs, MFGpuImage*, MF_MODEL_MAT_TEXTURE_DIFFUSE)
    };

    MFPipelineConfig info = {
        .extent = (MFVec2){ .x = state->sceneViewport.x, .y = state->sceneViewport.y },
        .hasDepth = true,
        .depthCompareOp = MF_COMPARE_OP_LESS,
        .transparent = false,
        .vertPath = "shaders/default.vert.spv",
        .fragPath = "shaders/default.frag.spv",
        .attribDescsCount = attribCount,
        .attribDescs = attribDescs,
        .bindingDescsCount = bindingCount,
        .bindingDescs = &bindingDesc,
        .imgCount = imageCount,
        .images = images,
        .buffCount = mfGetRendererFramesInFlight() * 2,
        .buffers = state->ubos,
        .pass = mfRenderTargetGetPass(state->rt)
    };
    mfPipelineInit(state->pipeline, state->renderer, &info);

    MF_FREEMEM(attribDescs);
}

static void ResizeCallback(void* pstate) {
    MFTState* state = (MFTState*)pstate;
    
    // Updating the viewport size but not really necessary! This callback is only for demonstration!
    mfPipelineBind(state->pipeline, mfRendererGetViewport(state->renderer), mfRendererGetScissor(state->renderer));
}

#pragma endregion

static void renderEntity(MFEntity* e, MFScene* scene, void* pstate) {
    MFTState* state = (MFTState*)pstate;

    UBOData uboData = {
        .proj = state->camera.proj,
        .view = state->camera.view,
        .normalMat = mfMat4Identity(),
        .model = mfMat4Identity()
    };
    
    MFMeshComponent* mcomp = mfSceneEntityGetMeshComponent(scene, e->id);
    MFTransformComponent* tcomp = mfSceneEntityGetTransformComponent(scene, e->id);

    {
        f64 time = mfGetCurrentTime();
        MFMat4 transformMat = mfMat4Translate(tcomp->position.x, tcomp->position.y, tcomp->position.z);
        MFMat4 rot = mfMat4RotateXYZ(tcomp->rotationXYZ.x * MF_DEG2RAD_MULTIPLIER + time, tcomp->rotationXYZ.y * MF_DEG2RAD_MULTIPLIER + time, tcomp->rotationXYZ.z * MF_DEG2RAD_MULTIPLIER);
        MFMat4 scale = mfMat4Identity();
        mfMat4Scale(&scale, tcomp->scale.x, tcomp->scale.y, tcomp->scale.z);

        uboData.model = mfMat4Mul(transformMat, mfMat4Mul(rot, scale));
        uboData.normalMat = mfMat4Transpose(mfMat4Inverse(mfMat4Mul(uboData.view, uboData.model)));
        mfGpuBufferUploadData(state->ubos[mfGetRendererCurrentFrameIdx(scene->renderer)], &uboData);
        mfGpuBufferUploadData(state->ubos[mfGetRendererCurrentFrameIdx(scene->renderer) + mfGetRendererFramesInFlight()], &state->lightData);
    }
    
    for(u64 i = 0; i < mcomp->model.meshCount; i++) {
        state->lightData.camPos = state->camera.pos;

        MFViewport vp = mfRendererGetViewport(scene->renderer);
        MFRect2D scissor = mfRendererGetScissor(scene->renderer);

        mfPipelineBind(state->pipeline, vp, scissor);
        mfMeshRender(&mcomp->model.meshes[i]);
    }
}

#pragma region MFTest

void MFTOnInit(void* pstate, void* pappState) {
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfGetWindowConfig(appState->window);
    MFTState* state = (MFTState*)pstate;
   
    slogLoggerReset(&state->logger);
    slogLoggerSetName(&state->logger, "MFTest");
    INFO(&state->logger, "MFTest init");

    state->renderer = appState->renderer;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));

    mfCameraCreate(&state->camera, appState->window, winConfig->width, winConfig->height, 60, 0.01f, 1000.0f, 0.025f, 0.075f, (MFVec3){0.0f, 0.0f, 2.0f});

    // Viewport and render target
    {
        state->rt = MF_ALLOCMEM(MFRenderTarget, mfGetRenderTargetSizeInBytes());
        mfRenderTargetCreate(state->rt, appState->renderer, true);
        mfRenderTargetSetResizeCallback(state->rt, &ResizeCallback, state);
        mfRendererSetRenderTarget(appState->renderer, state->rt);

        state->sceneViewport.x = mfRenderTargetGetWidth(state->rt);
        state->sceneViewport.y = mfRenderTargetGetHeight(state->rt);
    }
    // Scene & entities
    {
        mfSceneCreate(&state->scene, state->camera, appState->renderer);
        if(!mfSceneDeserialize(&state->scene, "./mftscene.bin", &vertBuilder)) {
            state->entity = mfSceneCreateEntity(&state->scene);

            MFMeshComponent mComp = {
                .path = "meshes/moon_rock.obj",
                .perVertSize = sizeof(Vertex),
                .vertBuilder = vertBuilder
            };

            MFTransformComponent tComp = {
                .position = (MFVec3){0, 0, 0},
                .rotationXYZ = (MFVec3){45, 0, 0},
                .scale = (MFVec3){15, 15, 15}
            };

            mfSceneEntityAddMeshComponent(&state->scene, state->entity->id, mComp);
            mfSceneEntityAddTransformComponent(&state->scene, state->entity->id, tComp);
        } else {
            state->entity = &mfArrayGet(state->scene.entities, MFEntity, 0);
        }
    }
    // UBO
    {
        state->ubos = MF_ALLOCMEM(MFGpuBuffer*, sizeof(MFGpuBuffer*) * mfGetRendererFramesInFlight() * 2);
        for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
            state->ubos[i] = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        }
        for(u8 i = mfGetRendererFramesInFlight(); i < mfGetRendererFramesInFlight() * 2; i++) {
            state->ubos[i] = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        }

        MFGpuBufferConfig config = {
            .type = MF_GPU_BUFFER_TYPE_UBO,
            .size = sizeof(UBOData),
            .binding = 0,
            .stage = MF_SHADER_STAGE_VERTEX
        };

        UBOData uboData = {
            .proj = state->camera.proj,
            .view = state->camera.view,
            .model = mfMat4Identity(),
            .normalMat = mfMat4Identity()
        };

        for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
            mfGpuBufferAllocate(state->ubos[i], config, appState->renderer);
            mfGpuBufferUploadData(state->ubos[i], &uboData);
        }

        config.size = sizeof(LightUBOData);
        config.binding = 1;
        config.stage = MF_SHADER_STAGE_FRAGMENT;

        state->lightData = (LightUBOData) {
            .ambientFactor = 0.01f,
            .camPos = state->camera.pos,
            .lightPos = (MFVec3){0.0f, 20.0f, 20.0f},
            .lightColor = (MFVec3){1.0f, 1.0f, 1.0f},
            .specularFactor = 32,
            .lightIntensity = 100
        };

        for(u8 i = mfGetRendererFramesInFlight(); i < mfGetRendererFramesInFlight() * 2; i++) {
            mfGpuBufferAllocate(state->ubos[i], config, appState->renderer);
            mfGpuBufferUploadData(state->ubos[i], &state->lightData);
        }
    }
    // Model Images
    {
        MFMeshComponent* comp = mfSceneEntityGetMeshComponent(&state->scene, state->entity->id);
        state->modelMatImgs = mfMaterialSystemGetModelMatImages(&comp->model, "meshes", state->renderer);
        mfGpuImageSetBinding(mfArrayGet(state->modelMatImgs, MFGpuImage*, MF_MODEL_MAT_TEXTURE_DIFFUSE), 2);
    }
    
    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    CreatePipeline(state);

    // UI customization
    SetUiStyle();
}

void MFTOnDeinit(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    
    INFO(&state->logger, "MFTest deinit");
    slogLoggerReset(&state->logger);

    mfSceneSerialize(&state->scene, "./mftscene.bin");

    mfMaterialSystemDeleteModelMatImages(&state->modelMatImgs);
    
    for(u8 i = 0; i < mfGetRendererFramesInFlight(); i++) {
        mfGpuBufferFree(state->ubos[i]);
        MF_FREEMEM(state->ubos[i]);
    }
    for(u8 i = mfGetRendererFramesInFlight(); i < mfGetRendererFramesInFlight() * 2; i++) {
        mfGpuBufferFree(state->ubos[i]);
        MF_FREEMEM(state->ubos[i]);
    }

    mfSceneDeleteEntity(&state->scene, state->entity->id);
    mfSceneDestroy(&state->scene);

    mfRenderTargetDestroy(state->rt);

    mfCameraDestroy(&state->camera);
    mfPipelineDestroy(state->pipeline);

    MF_FREEMEM(state->rt);
    MF_FREEMEM(state->ubos);
    MF_FREEMEM(state->pipeline);
}

void MFTOnRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    if((state->sceneViewport.x != mfRenderTargetGetWidth(state->rt)) || (state->sceneViewport.y != mfRenderTargetGetHeight(state->rt))) {
        mfRenderTargetResize(state->rt, (MFVec2){state->sceneViewport.x, state->sceneViewport.y});
    }

    mfSceneRender(&state->scene, renderEntity, pstate);
    mfSceneUpdate(&state->scene);
}

void MFTOnUIRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    igDockSpaceOverViewport(igGetID_Str("Dockspace"), igGetMainViewport(), ImGuiDockNodeFlags_None, mfnull);

    // Scene window
    {
        igBegin("Scene", mfnull, ImGuiWindowFlags_None);
        igGetContentRegionAvail(&state->sceneViewport);
        igImage((ImTextureID)mfRenderTargetGetHandle(state->rt), (ImVec2){mfRenderTargetGetWidth(state->rt), mfRenderTargetGetHeight(state->rt)}, (ImVec2){0, 0}, (ImVec2){1, 1});
        igEnd();
    }

    // Perf window
    {
        igBegin("Performance", mfnull, ImGuiWindowFlags_None);
        
        igText("Frame time :- %.3f", mfGetRendererGetFrameTime(appState->renderer));
        igText("Delta time :- %.3f", mfGetRendererGetDeltaTime(appState->renderer));
        igText("FPS :- %.3f", (f64)(1000.0/mfGetRendererGetDeltaTime(appState->renderer)));

        igEnd();
    }

    // Settings window
    {
        igBegin("Settings", mfnull, ImGuiWindowFlags_None);

        float posData[3] = {0};
        mfCopyVec3ToFloatArr(posData, state->lightData.lightPos);

        float colorData[3] = {0};
        mfCopyVec3ToFloatArr(colorData, state->lightData.lightColor);

        igDragFloat3("LightPos", posData, 0.1f, -5000.0f, 5000.0f, mfnull, ImGuiSliderFlags_None);
        igDragFloat("Ambient Factor", &state->lightData.ambientFactor, 0.01f, 0.0f, 1.0f, mfnull, ImGuiSliderFlags_None);
        igDragFloat("Specular Factor", &state->lightData.specularFactor, 0.1f, 2.0f, 512.0f, mfnull, ImGuiSliderFlags_None);
        igDragFloat("lightIntensity", &state->lightData.lightIntensity, 0.5f, 1.0f, 10000.0f, mfnull, ImGuiSliderFlags_None);
        igColorEdit3("Light Color", colorData, ImGuiColorEditFlags_None);

        state->lightData.lightPos = mfCopyFloatArrToVec3(posData);
        state->lightData.lightColor = mfCopyFloatArrToVec3(colorData);

        igEnd();
    }
}

void MFTOnUpdate(void* pstate, void* pappState) {
    MFDefaultAppState* aState = (MFDefaultAppState*)pappState;
    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* winConfig = mfGetWindowConfig(aState->window);

    state->camera.width = state->sceneViewport.x;
    state->camera.height = state->sceneViewport.y;
    state->camera.update(&state->camera, mfGetRendererGetDeltaTime(aState->renderer), mfnull);

    if(mfInputIsKeyPressed(aState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(aState->window);
    }
}

#pragma endregion
