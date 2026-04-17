#include "mftest.h"
#include "util.h"

#define INFO(logger, msg, ...) slogLogMsg(logger, SLOG_SEVERITY_INFO, msg, ##__VA_ARGS__)

#pragma region Helpers

static void CreatePipeline(MFTState* state) {
    u32 attributeCount = 0, bindingCount = 1;
    MFVertexInputAttributeDescription* attributes = getVertAttribDescs(&attributeCount);
    MFVertexInputBindingDescription bindings = getVertBindingDesc();

    MFPushConstantRange range = {
        .offset = 0,
        .size = sizeof(PushConstantData),
        .stage = MF_SHADER_STAGE_VERTEX
    };

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
        .renderTarget = state->renderTarget,
        .pushConstRangeCount = 1,
        .pushConstRanges = &range
    };
    mfPipelineInit(state->pipeline, state->renderer, &info);

    MF_FREEMEM(attributes);
}

static void RenderTargetResizeCallback(void* pstate) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "Render target resize callback");

    MFTState* state = (MFTState*)pstate;
    
    state->scene.camera.width = state->sceneViewport.x;
    state->scene.camera.height = state->sceneViewport.y;
    state->scene.camera.constructMatrices(&state->scene.camera);

    MF_PROFILE_ZONE_END(__temp);
}

static void MeshCallback(void* _state, MFMat4 transform, const MFMeshComponent* component, u64 meshIdx, MFPipeline* pipeline) {
    MFTState* state = (MFTState*)_state;

    PushConstantData modelData = {
        .model = transform
    };

    modelData.normalMat = mfMat4ToMat3(mfMat4Transpose(mfMat4Inverse(modelData.model)));

    mfResourceSetBind(state->sets[meshIdx], state->pipeline);
    mfPipelinePushConstant(state->pipeline, MF_SHADER_STAGE_VERTEX, 0, sizeof(PushConstantData), &modelData);
}

static MFMat4 ComputeModelMatrix(const MFTransformComponent* component) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "Computing model matrix");
    MFMat4 transformMat = mfMat4Translate(component->position.x, component->position.y, component->position.z);
    MFMat4 rotation = mfMat4RotateXYZ(component->rotationXYZ.x * MF_DEG2RAD_MULTIPLIER, component->rotationXYZ.y * MF_DEG2RAD_MULTIPLIER, component->rotationXYZ.z * MF_DEG2RAD_MULTIPLIER);
    MFMat4 scale = mfMat4Scale(fmax(component->scale.x, 0.5f), fmax(component->scale.y, 0.5f), fmax(component->scale.z, 0.5f));

    MFMat4 model = mfMat4Mul(transformMat, mfMat4Mul(rotation, scale));
    
    MF_PROFILE_ZONE_END(__temp);
    return model;
}

static void CreateResourceHandles(MFTState* state, MFDefaultAppState* appState) {
    // Resource layouts
    {
        state->layout = MF_ALLOCMEM(MFResourceSetLayout, mfResourceSetLayoutGetSizeInBytes());
        
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entity);
        MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages, &component->model, 0, appState->renderer);
        MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages, &component->model, 0, appState->renderer);
        
        MFResourceDescription descs[] = {
            mfGpuImageGetDescription(diffuseImage), // NOTE: Description for one image is enough since they have the same bindings 
            mfGpuImageGetDescription(normalImage), // NOTE: Description for one image is enough since they have the same bindings 
            mfGpuBufferGetDescription(state->cameraUbo),
            mfGpuBufferGetDescription(state->lightUbo)
        };
        
        mfResourceSetLayoutCreate(state->layout, MF_ARRAYLEN(descs, MFResourceDescription), descs, component->model.meshCount, appState->renderer);
    }
    // Resource sets
    {
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entity);
        state->setCount = component->model.meshCount;

        state->sets = MF_ALLOCMEM(MFResourceSet*, sizeof(MFResourceSet*) * state->setCount);
        MFArray buffers = mfArrayCreate(&state->logger, 2, sizeof(MFGpuBuffer*));
        mfArrayAddElement(buffers, MFGpuBuffer*, &state->logger, state->cameraUbo);
        mfArrayAddElement(buffers, MFGpuBuffer*, &state->logger, state->lightUbo);

        for(u64 i = 0; i < state->setCount; i++) {
            state->sets[i] = MF_ALLOCMEM(MFResourceSet, mfResourceSetGetSizeInBytes());
            mfResourceSetCreate(state->sets[i], state->layout, appState->renderer);

            MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages, &component->model, i, appState->renderer);
            MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages, &component->model, i, appState->renderer);
            
            MFArray images = mfArrayCreate(&state->logger, 2, sizeof(MFGpuImage*));
            mfArrayAddElement(images, MFGpuImage*, &state->logger, diffuseImage);
            mfArrayAddElement(images, MFGpuImage*, &state->logger, normalImage);

            mfResourceSetUpdate(state->sets[i], &images, &buffers);
            
            mfArrayDestroy(&images, &state->logger);
        }
        
        mfArrayDestroy(&buffers, &state->logger);
    }
}

static void CreateUBOs(MFTState* state, MFDefaultAppState* appState) {
    MFGpuBufferConfig config = {
        .type = MF_GPU_BUFFER_TYPE_UBO,
        .size = sizeof(UBOData),
        .binding = 0,
        .stage = MF_SHADER_STAGE_VERTEX
    };

    state->cameraUboData.proj = state->scene.camera.proj;
    state->cameraUboData.view = state->scene.camera.view;

    state->cameraUbo = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
    mfGpuBufferAllocate(state->cameraUbo, config, appState->renderer);
    mfGpuBufferUploadData(state->cameraUbo, &state->cameraUboData);
    
    config.size = sizeof(LightUBOData);
    config.binding = 1;
    config.stage = MF_SHADER_STAGE_FRAGMENT;
    
    state->lightData = (LightUBOData) {
        .ambientFactor = 0.01f,
        .camPos = state->scene.camera.pos,
        .lightPos = (MFVec3){0.0f, 2.0f, 0.0f},
        .lightColor = (MFVec3){1.0f, 1.0f, 1.0f},
        .specularFactor = 128,
        .lightIntensity = 100,
        .isPoint = 1.0f
    };
    
    state->lightUbo = MF_ALLOCMEM(MFGpuBuffer, mfGpuBufferGetSizeInBytes());
    mfGpuBufferAllocate(state->lightUbo, config, appState->renderer);
    mfGpuBufferUploadData(state->lightUbo, &state->lightData);
}

static void ConfigModelImages(MFTState* state, MFDefaultAppState* appState) {
    MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entity);
    char* basePath = mfnull;
    b8 noBasePath = false;
    {
        i32 idx = mfStringFindLast(&state->logger, component->path, '\\');
        if(idx == -1) {
            idx = mfStringFindLast(&state->logger, component->path, '/');
            if(idx == -1) {
                noBasePath = true;
                basePath = MF_ALLOCMEM(char, sizeof(char) * 3);
                basePath[0] = '.';
                basePath[1] = '/';
                basePath[2] = '\0';
            }
        }

        if(!noBasePath) {
            basePath = mfStringSliceRight(&state->logger, component->path, idx);
        }
    }

    state->materialImages = mfMaterialSystemLoadModelMatImages(&component->model, basePath, state->renderer);
    for(u64 i = 0; i < component->model.meshCount; i++) {
        MFGpuImage* image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages, &component->model, i, appState->renderer);
        mfGpuImageSetBinding(image, 2);
        image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages, &component->model, i, appState->renderer);
        mfGpuImageSetBinding(image, 3);
    }

    MF_FREEMEM(basePath);
}

static void CreateScene(MFTState* state, MFDefaultAppState* appState) {
    MFCamera camera;
    mfCameraCreate(&camera, appState->window, mfWindowGetConfig(appState->window)->width, mfWindowGetConfig(appState->window)->height, 60, 0.01f, 1000.0f, 0.025f, 0.075f, (MFVec3){0.0f, 0.0f, 2.0f});
    mfSceneCreate(&state->scene, camera, appState->renderer);
    if(!mfSceneDeserialize(&state->scene, "./mftscene.bin", &vertBuilder)) {
        state->entity = mfSceneCreateEntity(&state->scene);

        MFMeshComponent mComp = {
            .path = "meshes/Sponza/glTF/Sponza.gltf",
            .perVertSize = sizeof(Vertex),
            .vertBuilder = vertBuilder
        };

        MFTransformComponent tComp = {
            .position = (MFVec3){0, 0, 0},
            .rotationXYZ = (MFVec3){0, 0, 0},
            .scale = (MFVec3){1, 1, 1}
        };

        mfSceneEntityAddMeshComponent(&state->scene, &state->entity, mComp);
        mfSceneEntityAddTransformComponent(&state->scene, &state->entity, tComp);

    } else {
        u64 entityCount = 0;
        mfSceneGetValidEntities(&state->scene, &entityCount, mfnull);
        MFEntity* entities = MF_ALLOCMEM(MFEntity, sizeof(MFEntity) * entityCount);
        mfSceneGetValidEntities(&state->scene, &entityCount, entities);

        state->entity = entities[0].id;

        MF_FREEMEM(entities);
    }
}

#pragma endregion

#pragma region MFTest

void MFTOnInit(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest init");

    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfWindowGetConfig(appState->window);
    MFTState* state = (MFTState*)pstate;
   
    slogLoggerCreate(&state->logger, "MFTest", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE);
    slogLoggerSetName(&state->logger, "MFTest");
    INFO(&state->logger, "MFTest init");

    state->renderer = appState->renderer;
    mfRendererSetClearColor(appState->renderer, mfVec3Create(0, 0, 0.01f));

    // Viewport and render target
    {
        state->renderTarget = MF_ALLOCMEM(MFRenderTarget, mfRenderTargetGetSizeInBytes());
        mfRenderTargetCreate(state->renderTarget, appState->renderer, true);
        mfRenderTargetSetResizeCallback(state->renderTarget, &RenderTargetResizeCallback, state);
        mfRendererSetRenderTarget(appState->renderer, state->renderTarget);

        state->sceneViewport.x = mfRenderTargetGetWidth(state->renderTarget);
        state->sceneViewport.y = mfRenderTargetGetHeight(state->renderTarget);
    }

    CreateScene(state, appState);
    ConfigModelImages(state, appState);
    CreateUBOs(state, appState);
    CreateResourceHandles(state, appState);

    state->pipeline = MF_ALLOCMEM(MFPipeline, mfPipelineGetSizeInBytes());
    CreatePipeline(state);

    SetUiStyle();

    MF_PROFILE_ZONE_END(__temp);
}

void MFTOnDeinit(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    
    INFO(&state->logger, "MFTest deinit");
    slogLoggerDestroy(&state->logger);

    for(u64 i = 0; i < state->setCount; i++) {
        mfResourceSetDestroy(state->sets[i]);
    }
    mfResourceSetLayoutDestroy(state->layout);

    mfMaterialSystemDestroyModelMatImages(&state->materialImages);
    
    mfGpuBufferFree(state->cameraUbo);
    mfGpuBufferFree(state->lightUbo);

    mfSceneSerialize(&state->scene, "./mftscene.bin");
    mfSceneDeleteEntity(&state->scene, &state->entity);
    mfSceneDestroy(&state->scene);

    mfRenderTargetDestroy(state->renderTarget);

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

    MFSceneRenderConfig config = {
        .state = state,
        .entityPipeline = state->pipeline,
        .scissor = mfRendererGetScissor(state->renderer),
        .viewport = mfRendererGetViewport(state->renderer),
        .perMeshDrawCallback = &MeshCallback,
        .computeModelMatrix = &ComputeModelMatrix
    };
    mfSceneRender(&state->scene, &config);
}

void MFTOnUIRender(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    igDockSpaceOverViewport(igGetID_Str("Dockspace"), igGetMainViewport(), ImGuiDockNodeFlags_None, mfnull);

    // Scene window
    {
        igBegin("Scene", mfnull, ImGuiWindowFlags_None);
        igGetContentRegionAvail(&state->sceneViewport);
        igImage(mfRenderTargetGetImGuiTextureID(state->renderTarget), (ImVec2){mfRenderTargetGetWidth(state->renderTarget), mfRenderTargetGetHeight(state->renderTarget)}, (ImVec2){0, 0}, (ImVec2){1, 1});
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

        if(igCollapsingHeader_BoolPtr("Light settings", mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
            f32 posData[3] = {0};
            mfCopyVec3ToFloatArr(posData, state->lightData.lightPos);

            f32 colorData[3] = {0};
            mfCopyVec3ToFloatArr(colorData, state->lightData.lightColor);

            igDragFloat3("LightPos", posData, 0.1f, -5000.0f, 5000.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igDragFloat("Ambient Factor", &state->lightData.ambientFactor, 0.01f, 0.0f, 1.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igDragFloat("Specular Factor", &state->lightData.specularFactor, 0.1f, 2.0f, 512.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igDragFloat("Light Intensity", &state->lightData.lightIntensity, 0.5f, 1.0f, 10000.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igColorEdit3("Light Color", colorData, ImGuiColorEditFlags_None);
            
            bool isPoint = state->lightData.isPoint;
            igCheckbox("Point lighting", &isPoint);

            state->lightData.lightPos = mfFloatArrToVec3(posData);
            state->lightData.lightColor = mfFloatArrToVec3(colorData);
            state->lightData.isPoint = isPoint;
        }

        igDummy((ImVec2){ 0.0f, 50.0f });

        if(igCollapsingHeader_BoolPtr("Model transform settings", mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
            MFTransformComponent* transformComponent = mfSceneEntityGetTransformComponent(&state->scene, &state->entity);
            
            f32 scale[3] = {0};
            mfCopyVec3ToFloatArr(scale, transformComponent->scale);
            f32 position[3] = {0};
            mfCopyVec3ToFloatArr(position, transformComponent->position);
            f32 rotation[3] = {0};
            mfCopyVec3ToFloatArr(rotation, transformComponent->rotationXYZ);
            
            igDragFloat3("Postion", position, 0.1f, -1e6f, 1e6f, mfnull, ImGuiSliderFlags_None);
            igDragFloat3("Scale", scale, 0.1f, 1.0f, 1e6f, mfnull, ImGuiSliderFlags_None);
            igDragFloat3("Rotation (In degrees)", rotation, 0.1f, 0.0f, 360.0f, mfnull, ImGuiSliderFlags_None);
        
            transformComponent->position = mfFloatArrToVec3(position);
            transformComponent->scale = mfFloatArrToVec3(scale);
            transformComponent->rotationXYZ = mfFloatArrToVec3(rotation);
        }

        igEnd();
    }
}

void MFTOnUpdate(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest update");

    MFDefaultAppState* aState = (MFDefaultAppState*)pappState;
    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* winConfig = mfWindowGetConfig(aState->window);

    state->scene.camera.width = state->sceneViewport.x;
    state->scene.camera.height = state->sceneViewport.y;
    state->scene.camera.update(&state->scene.camera, mfRendererGetDeltaTime(aState->renderer), mfnull);

    state->cameraUboData.proj = state->scene.camera.proj;
    state->cameraUboData.view = state->scene.camera.view;
    state->lightData.camPos = state->scene.camera.pos;
    mfGpuBufferUploadData(state->lightUbo, &state->lightData);
    mfGpuBufferUploadData(state->cameraUbo, &state->cameraUboData);

    if(mfInputIsKeyPressed(aState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(aState->window);
    }
    
    MF_PROFILE_ZONE_END(__temp);
}

#pragma endregion
