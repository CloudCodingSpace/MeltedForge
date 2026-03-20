#include "mftest.h"
#include "core/mfmaths.h"
#include "ecs/mfscene.h"
#include "renderer/mfrenderer.h"
#include "slog/slog.h"
#include "util.h"

#define INFO(logger, msg, ...) slogLogMsg(logger, SLOG_SEVERITY_INFO, msg, ##__VA_ARGS__)

static void CreatePipeline(MFTState* state) {
    u32 attributeCount = 0, bindingCount = 1;
    MFVertexInputAttributeDescription* attributes = getVertAttribDescs(&attributeCount);
    MFVertexInputBindingDescription bindings = getVertBindingDesc();

    MFPipelineConfig info = {
        .extent = (MFVec2){ .x = state->sceneViewport.x, .y = state->sceneViewport.y },
        .hasDepth = true,
        .depthCompareOp = MF_COMPARE_OP_LESS,
        .transparent = false,
        .vertPath = "shaders/default.vert.spv",
        .fragPath = "shaders/default.frag.spv",
        .attributesCount = attributeCount,
        .attributes = attributes,
        .bindingsCount = bindingCount,
        .bindings = &bindings,
        .resourceLayoutCount = 1,
        .resourceLayouts = &state->layout,
        .renderpass = mfRenderTargetGetPass(state->renderTarget)
    };
    mfPipelineInit(state->pipeline, state->renderer, &info);

    MF_FREEMEM(attributes);
}

static void ResizeCallback(void* pstate) {
    MFTState* state = (MFTState*)pstate;
    
    // Updating the viewport size but not really necessary for pipelines! This callback is only for demonstration!
    mfPipelineDestroy(state->pipeline);
    CreatePipeline(state);
}

static void renderEntity(MFEntity* e, MFScene* scene, void* pstate) {
    MFTState* state = (MFTState*)pstate;

    UBOData uboData = {
        .proj = state->camera.proj,
        .view = state->camera.view,
        .normalMat = mfMat4Identity(),
        .model = mfMat4Identity()
    };
    
    MFMeshComponent* mcomponent = mfSceneEntityGetMeshComponent(scene, e->id);
    MFTransformComponent* tcomponent = mfSceneEntityGetTransformComponent(scene, e->id);

    {
        f64 time = mfGetTimeElapsed();
        MFMat4 transformMat = mfMat4Translate(tcomponent->position.x, tcomponent->position.y, tcomponent->position.z);
        MFMat4 rot = mfMat4RotateXYZ(tcomponent->rotationXYZ.x * MF_DEG2RAD_MULTIPLIER + time, tcomponent->rotationXYZ.y * MF_DEG2RAD_MULTIPLIER + time, tcomponent->rotationXYZ.z * MF_DEG2RAD_MULTIPLIER);
        MFMat4 scale = mfMat4Identity();
        mfMat4Scale(&scale, tcomponent->scale.x, tcomponent->scale.y, tcomponent->scale.z);

        uboData.model = mfMat4Mul(transformMat, mfMat4Mul(rot, scale));
        uboData.normalMat = mfMat4Transpose(mfMat4Inverse(mfMat4Mul(uboData.view, uboData.model)));
        mfGpuBufferUploadData(state->cameraUbo, &uboData);
        mfGpuBufferUploadData(state->lightUbo, &state->lightData);
    }
    
    for(u64 i = 0; i < mcomponent->model.meshCount; i++) {
        state->lightData.camPos = state->camera.pos;

        MFViewport vp = mfRendererGetViewport(scene->renderer);
        MFRect2D scissor = mfRendererGetScissor(scene->renderer);

        mfResourceSetBind(state->sets[i], state->pipeline);
        mfPipelineBind(state->pipeline, vp, scissor);
        mfMeshRender(&mcomponent->model.meshes[i]);
    }
}

#pragma region MFTest

void MFTOnInit(void* pstate, void* pappState) {
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfWindowGetConfig(appState->window);
    MFTState* state = (MFTState*)pstate;
   
    slogLoggerCreate(&state->logger, "MFTest", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE);
    slogLoggerSetName(&state->logger, "MFTest");
    INFO(&state->logger, "MFTest init");

    state->renderer = appState->renderer;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0.1f, 0.1f, 0.1f));

    mfCameraCreate(&state->camera, appState->window, winConfig->width, winConfig->height, 60, 0.01f, 1000.0f, 0.025f, 0.075f, (MFVec3){0.0f, 0.0f, 2.0f});

    // Viewport and render target
    {
        state->renderTarget = MF_ALLOCMEM(MFRenderTarget, mfRenderTargetGetSizeInBytes());
        mfRenderTargetCreate(state->renderTarget, appState->renderer, true);
        mfRenderTargetSetResizeCallback(state->renderTarget, &ResizeCallback, state);
        mfRendererSetRenderTarget(appState->renderer, state->renderTarget);

        state->sceneViewport.x = mfRenderTargetGetWidth(state->renderTarget);
        state->sceneViewport.y = mfRenderTargetGetHeight(state->renderTarget);
    }
    // Scene & entities
    {
        mfSceneCreate(&state->scene, state->camera, appState->renderer);
        if(!mfSceneDeserialize(&state->scene, "./mftscene.bin", &vertBuilder)) {
            state->entity = mfSceneCreateEntity(&state->scene);

            MFMeshComponent mComp = {
                .path = "meshes/Mickey Mouse.obj",
                .perVertSize = sizeof(Vertex),
                .vertBuilder = vertBuilder
            };

            MFTransformComponent tComp = {
                .position = (MFVec3){0, 0, 0},
                .rotationXYZ = (MFVec3){45, 0, 0},
                .scale = (MFVec3){1, 1, 1}
            };

            mfSceneEntityAddMeshComponent(&state->scene, state->entity->id, mComp);
            mfSceneEntityAddTransformComponent(&state->scene, state->entity->id, tComp);
        } else {
            state->entity = &mfArrayGet(state->scene.entities, MFEntity, 0);
        }
    }
    // UBO
    {
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

        state->cameraUbo = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        mfGpuBufferAllocate(state->cameraUbo, config, appState->renderer);
        mfGpuBufferUploadData(state->cameraUbo, &uboData);
        
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
        
        state->lightUbo = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
        mfGpuBufferAllocate(state->lightUbo, config, appState->renderer);
        mfGpuBufferUploadData(state->lightUbo, &state->lightData);
    }
    // Model Images
    {
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, state->entity->id);
        state->materialImages = mfMaterialSystemLoadModelMatImages(&component->model, "meshes", state->renderer);
        for(u64 i = 0; i < component->model.meshCount; i++) {
            MFGpuImage* image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages, &component->model, i, appState->renderer);
            mfGpuImageSetBinding(image, 2);
        }
    }
    // Resource layouts
    {
        state->layout = MF_ALLOCMEM(MFResourceSetLayout, mfResourceSetLayoutGetSizeInBytes());
        
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, state->entity->id);
        u64 count = 3;
        
        MFResourceDesc* descs = MF_ALLOCMEM(MFResourceDesc, sizeof(MFResourceDesc) * count);
        
        u64 i = 0;
        MFGpuImage* image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages, &component->model, 0, appState->renderer); // anyone image is fine since every image has same binding
        descs[i++] = mfGpuImageGetDescription(image);
        descs[i++] = mfGpuBufferGetDescription(state->cameraUbo);
        descs[i++] = mfGpuBufferGetDescription(state->lightUbo);
        
        mfResourceSetLayoutCreate(state->layout, count, descs, appState->renderer);
        
        MF_FREEMEM(descs);
    }
    // Resource sets
    {
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, state->entity->id);
        state->setCount = component->model.meshCount;

        state->sets = MF_ALLOCMEM(MFResourceSet*, sizeof(MFResourceSet*) * state->setCount);
        MFArray buffers = mfArrayCreate(&state->logger, 2, sizeof(MFGpuBuffer*));
        mfArrayAddElement(buffers, MFGpuBuffer*, &state->logger, state->cameraUbo);
        mfArrayAddElement(buffers, MFGpuBuffer*, &state->logger, state->lightUbo);

        for(u64 i = 0; i < state->setCount; i++) {
            state->sets[i] = MF_ALLOCMEM(MFResourceSet, mfResourceSetGetSizeInBytes());
            mfResourceSetCreate(state->sets[i], state->layout, appState->renderer);

            MFGpuImage* image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages, &component->model, i, appState->renderer);
            MFArray images = mfArrayCreate(&state->logger, 1, sizeof(MFGpuImage*));
            mfArrayAddElement(images, MFGpuImage*, &state->logger, image);

            mfResourceSetUpdate(state->sets[i], &images, &buffers);
            
            mfArrayDestroy(&images, &state->logger);
        }
        
        mfArrayDestroy(&buffers, &state->logger);
    }

    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    CreatePipeline(state);

    // UI customization
    SetUiStyle();
}

void MFTOnDeinit(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    
    INFO(&state->logger, "MFTest deinit");
    slogLoggerDestroy(&state->logger);

    for(u64 i = 0; i < state->setCount; i++) {
        mfResourceSetDestroy(state->sets[i]);
    }
    mfResourceSetLayoutDestroy(state->layout);

    mfSceneSerialize(&state->scene, "./mftscene.bin");

    mfMaterialSystemDeleteModelMatImages(&state->materialImages);
    
    mfGpuBufferFree(state->cameraUbo);
    mfGpuBufferFree(state->lightUbo);

    mfSceneDeleteEntity(&state->scene, state->entity->id);
    mfSceneDestroy(&state->scene);

    mfRenderTargetDestroy(state->renderTarget);

    mfCameraDestroy(&state->camera);
    mfPipelineDestroy(state->pipeline);

    for(u64 i = 0; i < state->setCount; i++) {
        MF_FREEMEM(state->sets[i]);
    }
    MF_FREEMEM(state->sets);
    MF_FREEMEM(state->layout);
    MF_FREEMEM(state->cameraUbo);
    MF_FREEMEM(state->lightUbo);
    MF_FREEMEM(state->renderTarget);
    MF_FREEMEM(state->pipeline);
}

void MFTOnRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    if((state->sceneViewport.x != mfRenderTargetGetWidth(state->renderTarget)) || (state->sceneViewport.y != mfRenderTargetGetHeight(state->renderTarget))) {
        mfRenderTargetResize(state->renderTarget, (MFVec2){state->sceneViewport.x, state->sceneViewport.y});
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
        igImage((ImTextureID)mfRenderTargetGetHandle(state->renderTarget), (ImVec2){mfRenderTargetGetWidth(state->renderTarget), mfRenderTargetGetHeight(state->renderTarget)}, (ImVec2){0, 0}, (ImVec2){1, 1});
        igEnd();
    }

    // Perf window
    {
        igBegin("Performance", mfnull, ImGuiWindowFlags_None);
        
        igText("Frame time :- %.3f", mfRendererGetFrameTime(appState->renderer));
        igText("Delta time :- %.3f", mfRendererGetDeltaTime(appState->renderer));
        igText("FPS :- %.3f", (f64)(1000.0/mfRendererGetDeltaTime(appState->renderer)));

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
    const MFWindowConfig* winConfig = mfWindowGetConfig(aState->window);

    state->camera.width = state->sceneViewport.x;
    state->camera.height = state->sceneViewport.y;
    state->camera.update(&state->camera, mfRendererGetDeltaTime(aState->renderer), mfnull);

    if(mfInputIsKeyPressed(aState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(aState->window);
    }
}

#pragma endregion
