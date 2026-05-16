#include "mftest.h"
#include "util.h"

#define INFO(logger, msg, ...) slogLogMsg(logger, SLOG_SEVERITY_INFO, msg, ##__VA_ARGS__)

#pragma region Helpers

static void CreatePipeline(MFTState* state) {
    u32 attributeCount = 0, bindingCount = 1;
    const MFWindowConfig* config = mfWindowGetConfig(state->window);
    MFVertexInputAttributeDescription* attributes = getVertAttribDescs(&attributeCount);
    MFVertexInputBindingDescription bindings = getVertBindingDesc();

    MFPushConstantRange range = {
        .offset = 0,
        .size = sizeof(PushConstantData),
        .stage = MF_SHADER_STAGE_VERTEX
    };

    MFResourceSetLayout* layouts[] = {
        state->layout,
        state->layout2
    };

    MFPipelineConfig info = {
        .extent = (MFVec2){ .x = config->width, .y = config->height },
        .hasDepth = true,
        .depthCompareOp = MF_COMPARE_OP_DEFAULT,
        .transparent = true,
        .vertPath = "mftshaders/default.vert.spv",
        .fragPath = "mftshaders/default.frag.spv",
        .attributesCount = attributeCount,
        .attributes = attributes,
        .bindingsCount = bindingCount,
        .bindings = &bindings,
        .resourceLayoutCount = MF_ARRAYLEN(layouts, MFResourceSetLayout*),
        .resourceLayouts = layouts,
        .pushConstRangeCount = 1,
        .pushConstRanges = &range,
        .cullMode = MF_CULL_MODE_BACK_BIT
    };

    state->pipeline = mfPipelineCreate(state->renderer, &info);

    info.extent = (MFVec2){ .x = state->sceneViewport.x, .y = state->sceneViewport.y };
    info.renderTarget = state->renderTarget;

    state->pipeline2 = mfPipelineCreate(state->renderer, &info);

    MF_FREEMEM(attributes);
}

static void ResizeCallback(void* pstate) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "Resize callback");

    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* config = mfWindowGetConfig(state->window);
    
    if(state->enableRenderTarget) {
        state->scene.camera.width = state->sceneViewport.x;
        state->scene.camera.height = state->sceneViewport.y;
    } else {
        state->scene.camera.width = config->width;
        state->scene.camera.height = config->height;
    }

    state->scene.camera.constructMatrices(&state->scene.camera);

    MF_PROFILE_ZONE_END(__temp);
}

static void MeshCallback(void* _state, MFMat4 transform, const MFMeshComponent* component, u64 meshIdx, MFPipeline* pipeline) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest Mesh callback");
    MFTState* state = (MFTState*)_state;

    PushConstantData modelData = {
        .model = transform
    };

    modelData.normalMat = mfMat4ToMat3(mfMat4Transpose(mfMat4Inverse(modelData.model)));

    MFResourceSet** set = &component->model.meshes[meshIdx].mat.set;
    mfResourceSetsBind(0, 1, set, pipeline);
    mfPipelinePushConstant(pipeline, MF_SHADER_STAGE_VERTEX, 0, sizeof(PushConstantData), &modelData);

    MF_PROFILE_ZONE_END(__temp);
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

static void PipelineBindCallback(void* _state, MFPipeline* pipeline) {
    MFTState* state = (MFTState*)_state;

    mfResourceSetsBind(1, 1, &state->set2, pipeline);
}

static void CreateResourceHandles(MFTState* state, MFDefaultAppState* appState) {
    MFGpuImage* skyboxImage = mfSkyboxGetCubemapImage(state->skybox);
    MFGpuImage* irradianceMap = mfSkyboxGetIrradianceCubemapImage(state->skybox);
    MFGpuImage* prefilteredMap = mfSkyboxGetPrefilteredCubemapImage(state->skybox);
    MFGpuImage* brdfLut = mfSkyboxGetBRDFLUT(state->skybox);
    // Resource layouts
    {
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entities[0]);
        MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages[0], &component->model, 0, appState->renderer);
        MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages[0], &component->model, 0, appState->renderer);
        MFGpuImage* metallicRoughnessImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_METALNESS, &state->materialImages[0], &component->model, 0, appState->renderer);
        MFGpuImage* emissiveImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_EMISSIVE, &state->materialImages[0], &component->model, 0, appState->renderer);

        MFResourceDescription descs[] = {
            mfGpuImageGetDescription(diffuseImage), // NOTE: Description for one image is enough since they have the same bindings 
            mfGpuImageGetDescription(normalImage), // NOTE: Description for one image is enough since they have the same bindings 
            mfGpuImageGetDescription(metallicRoughnessImage), // NOTE: Description for one image is enough since they have the same bindings 
            mfGpuImageGetDescription(emissiveImage), // NOTE: Description for one image is enough since they have the same bindings 
            mfGpuBufferGetDescription(state->cameraUbo),
            mfGpuBufferGetDescription(state->lightUbo)
        };
        
        state->layout = mfResourceSetLayoutCreate(MF_ARRAYLEN(descs, MFResourceDescription), descs, component->model.meshCount, appState->renderer);
    
        // Skybox layout
        {
            mfGpuImageSetBinding(skyboxImage, 0);
            mfGpuImageSetBinding(irradianceMap, 1);
            mfGpuImageSetBinding(prefilteredMap, 2);
            mfGpuImageSetBinding(brdfLut, 3);
            MFResourceDescription descs2[] = {
                mfGpuImageGetDescription(skyboxImage),
                mfGpuImageGetDescription(irradianceMap),
                mfGpuImageGetDescription(prefilteredMap),
                mfGpuImageGetDescription(brdfLut)
            };
            state->layout2 = mfResourceSetLayoutCreate(MF_ARRAYLEN(descs2, MFResourceDescription), descs2, 1, state->renderer);
        }
    }
    // Resource sets
    {
        MFArray buffers = mfArrayCreate(2, sizeof(MFGpuBuffer*));
        mfArrayAddElement(&buffers, MFGpuBuffer*, state->cameraUbo);
        mfArrayAddElement(&buffers, MFGpuBuffer*, state->lightUbo);

        for(u64 k = 0; k < state->scene.meshCompPool.len; k++) {
            MFMeshComponent* component = &mfArrayGetElement(state->scene.meshCompPool, MFMeshComponent, k);
            for(u64 i = 0; i < component->model.meshCount; i++) {
                MFResourceSet* set = mfResourceSetCreate(state->layout, appState->renderer);

                MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* metallicRoughnessImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_METALNESS, &state->materialImages[0], &component->model, 0, appState->renderer);
                MFGpuImage* emissiveImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_EMISSIVE, &state->materialImages[0], &component->model, 0, appState->renderer);

                MFArray images = mfArrayCreate(2, sizeof(MFGpuImage*));
                mfArrayAddElement(&images, MFGpuImage*, diffuseImage);
                mfArrayAddElement(&images, MFGpuImage*, normalImage);
                mfArrayAddElement(&images, MFGpuImage*, metallicRoughnessImage);
                mfArrayAddElement(&images, MFGpuImage*, emissiveImage);

                mfResourceSetUpdate(set, &images, &buffers);

                mfArrayDestroy(&images);
                component->model.meshes[i].mat.set = set;
            }
        }
        
        mfArrayDestroy(&buffers);

        // Skybox set
        {
            state->set2 = mfResourceSetCreate(state->layout2, state->renderer);

            MFArray images = mfArrayCreate(1, sizeof(MFGpuImage*));
            mfArrayAddElement(&images, MFGpuImage*, skyboxImage);
            mfArrayAddElement(&images, MFGpuImage*, irradianceMap);
            mfArrayAddElement(&images, MFGpuImage*, prefilteredMap);
            mfArrayAddElement(&images, MFGpuImage*, brdfLut);
            mfResourceSetUpdate(state->set2, &images, mfnull);
            mfArrayDestroy(&images);
        }
    }
    for(u64 i = 0; i < state->entityCount; i++)
        mfMaterialSystemDestroyModelMatImages(&state->materialImages[i]);
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

    state->cameraUbo = mfGpuBufferAllocate(config, appState->renderer);
    mfGpuBufferUploadData(state->cameraUbo, &state->cameraUboData);
    
    config.size = sizeof(LightUBOData);
    config.binding = 1;
    config.stage = MF_SHADER_STAGE_FRAGMENT;
    
    state->lightData = (LightUBOData) {
        .camPos = state->scene.camera.pos,
        .lightPos = (MFVec3){0.0f, 20.0f, 10.0f},
        .lightColor = (MFVec3){1.0f, 1.0f, 1.0f},
        .lightIntensity = 100,
        .useNormalMap = true,
        .useAcesTonemapping = true
    };
    
    state->lightUbo = mfGpuBufferAllocate(config, appState->renderer);
    mfGpuBufferUploadData(state->lightUbo, &state->lightData);
}

static void ConfigModelImages(MFTState* state, MFDefaultAppState* appState) {
    state->materialImages = MF_ALLOCMEM(MFArray, sizeof(MFArray) * state->entityCount);
    for(u64 i = 0; i < state->entityCount; i++) {
        MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entities[i]);
        char* basePath = mfnull;
        bool noBasePath = false;
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
                basePath = mfStringSliceLeft(&state->logger, component->path, idx);
            }
        }

        state->materialImages[i] = mfMaterialSystemLoadModelMatImages(&component->model, basePath, state->renderer);
        for(u64 k = 0; k < component->model.meshCount; k++) {
            MFGpuImage* image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages[i], &component->model, k, appState->renderer);
            mfGpuImageSetBinding(image, 2);
            image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages[i], &component->model, k, appState->renderer);
            mfGpuImageSetBinding(image, 3);
            image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_METALNESS, &state->materialImages[i], &component->model, k, appState->renderer);
            mfGpuImageSetBinding(image, 4);
            image = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_EMISSIVE, &state->materialImages[i], &component->model, k, appState->renderer);
            mfGpuImageSetBinding(image, 5);
        }

        MF_FREEMEM(basePath);
    }
}

static void CreateScene(MFTState* state, MFDefaultAppState* appState) {
    MFCamera camera = {};
    mfCameraCreate(&camera, appState->window, mfWindowGetConfig(appState->window)->width, mfWindowGetConfig(appState->window)->height, 60, 0.01f, 1000.0f, 0.025f, 0.075f, (MFVec3){0.0f, 0.0f, 2.0f});
    mfSceneCreate(&state->scene, camera, &vertBuilder, appState->renderer);
    if(!mfSceneDeserialize(&state->scene, "./mftscene.bin")) {
        state->entities = MF_ALLOCMEM(u64, sizeof(u64) * 2);
        state->entities[0] = mfSceneCreateEntity(&state->scene);
        state->entities[1] = mfSceneCreateEntity(&state->scene);
        state->entityCount = 2;

        MFMeshComponent mComp = {
            .path = "mftmeshes/Damaged Helmet/DamagedHelmet.gltf",
            // .path = "mftmeshes/Sponza/glTF/Sponza.gltf",
            .perVertSize = sizeof(Vertex)
        };

        MFTransformComponent tComp = {
            .position = (MFVec3){0, 0, 0},
            .rotationXYZ = (MFVec3){0, 0, 0},
            .scale = (MFVec3){1, 1, 1}
        };

        mfSceneAddMeshComponent(&state->scene, &mComp);
        mfSceneAddTransformComponent(&state->scene, &tComp);

        mfSceneEntityAttachMeshComponent(&state->scene, &state->entities[0], &mComp);
        mfSceneEntityAttachTransformComponent(&state->scene, &state->entities[0], &tComp);

        tComp.position = (MFVec3){5, 0, 0};
        mfSceneAddTransformComponent(&state->scene, &tComp);

        mfSceneEntityAttachMeshComponent(&state->scene, &state->entities[1], &mComp);
        mfSceneEntityAttachTransformComponent(&state->scene, &state->entities[1], &tComp);
    } else {
        u64 entityCount = 0;
        mfSceneGetValidEntities(&state->scene, &entityCount, mfnull);
        MFEntity* entities = MF_ALLOCMEM(MFEntity, sizeof(MFEntity) * entityCount);
        mfSceneGetValidEntities(&state->scene, &entityCount, entities);

        state->entities = MF_ALLOCMEM(u64, sizeof(u64) * entityCount);
        for(u64 i = 0; i < entityCount; i++) {
            state->entities[i] = entities[i].id;
        }
        state->entityCount = entityCount;

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

    state->enableRenderTarget = true;
    state->renderer = appState->renderer;
    state->window = appState->window;
   
    slogLoggerCreate(&state->logger, "MFTest", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE);
    slogLoggerSetName(&state->logger, "MFTest");
    INFO(&state->logger, "MFTest init");

    mfRendererSetClearColor(appState->renderer, mfVec3Create(0, 0, 0.01f));
    mfRendererSetResizeCallback(appState->renderer, state, &ResizeCallback);

    // Viewport and render target
    {
        state->renderTarget = mfRenderTargetCreate(appState->renderer, true);
        mfRenderTargetSetClearColor(state->renderTarget, mfVec3Create(0, 0, 0.01f));
        mfRenderTargetSetResizeCallback(state->renderTarget, &ResizeCallback, state);

        state->sceneViewport.x = mfRenderTargetGetWidth(state->renderTarget);
        state->sceneViewport.y = mfRenderTargetGetHeight(state->renderTarget);
    }

    // Skybox
    {
        MFSkyboxConfig config = {
            .binding = 0,
            .faceSize = 512,
            .environmentPath = "mftskyboxes/1.hdr",
            .generatePbrMaps = true
        };
        state->skybox = mfSkyboxCreate(config, appState->renderer);
        config.renderTarget = state->renderTarget;
        state->skybox2 = mfSkyboxCreate(config, appState->renderer);
    }

    CreateScene(state, appState);
    ConfigModelImages(state, appState);
    CreateUBOs(state, appState);
    CreateResourceHandles(state, appState);
    CreatePipeline(state);

    SetUiStyle();

    MF_PROFILE_ZONE_END(__temp);
}

void MFTOnDeinit(void* pstate, void* pappState) {
    MFTState* state = (MFTState*)pstate;
    
    INFO(&state->logger, "MFTest deinit");
    slogLoggerDestroy(&state->logger);

    for(u64 i = 0; i < state->scene.meshCompPool.len; i++) {
        MFMeshComponent* component = &mfArrayGetElement(state->scene.meshCompPool, MFMeshComponent, i);
        for(u64 j = 0; j < component->model.meshCount; j++) {
            mfResourceSetDestroy(component->model.meshes[j].mat.set);
        }
    }
    mfResourceSetDestroy(state->set2);

    mfResourceSetLayoutDestroy(state->layout);
    mfResourceSetLayoutDestroy(state->layout2);
    
    mfGpuBufferFree(state->cameraUbo);
    mfGpuBufferFree(state->lightUbo);

    mfSceneSerialize(&state->scene, "./mftscene.bin");
    for(u64 i = 0; i < state->entityCount; i++)
        mfSceneDeleteEntity(&state->scene, &state->entities[i]);
    mfSceneDestroy(&state->scene);

    mfSkyboxDestroy(state->skybox2);
    mfSkyboxDestroy(state->skybox);

    mfRenderTargetDestroy(state->renderTarget);

    mfPipelineDestroy(state->pipeline);
    mfPipelineDestroy(state->pipeline2);

    MF_FREEMEM(state->entities);
    MF_FREEMEM(state->materialImages);
}

void MFTOnRender(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest render");

    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    if((state->sceneViewport.x != mfRenderTargetGetWidth(state->renderTarget)) || (state->sceneViewport.y != mfRenderTargetGetHeight(state->renderTarget))) {
        if(state->enableRenderTarget)
            mfRenderTargetResize(state->renderTarget, (MFVec2){state->sceneViewport.x, state->sceneViewport.y});
    }

    if(state->enableRenderTarget)
        mfRenderTargetBegin(state->renderTarget);

    MFSceneRenderConfig config = {
        .state = state,
        .entityPipeline = (state->enableRenderTarget) ? state->pipeline2 : state->pipeline,
        .scissor = mfRendererGetScissor(state->renderer),
        .viewport = mfRendererGetViewport(state->renderer),
        .perMeshDrawCallback = &MeshCallback,
        .computeModelMatrix = &ComputeModelMatrix,
        .pipelineBindCallback = &PipelineBindCallback
    };

    mfSceneRender(&state->scene, &config);

    if(state->enableRenderTarget) {
        mfSkyboxRender(state->skybox2, state->cameraUboData.proj, state->cameraUboData.view, mfMat4Identity(), MF_SKYBOX_TYPE_NORMAL);
        mfRenderTargetEnd(state->renderTarget);
    }
    else {
        mfSkyboxRender(state->skybox, state->cameraUboData.proj, state->cameraUboData.view, mfMat4Identity(), MF_SKYBOX_TYPE_NORMAL);
    }

    MF_PROFILE_ZONE_END(__temp);
}

void MFTOnUIRender(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest update");

    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    // Scene window
    if(state->enableRenderTarget) {
        igDockSpaceOverViewport(igGetID_Str("Dockspace"), igGetMainViewport(), ImGuiDockNodeFlags_None, mfnull);

        igBegin("Scene", mfnull, ImGuiWindowFlags_None);
        igGetContentRegionAvail(&state->sceneViewport);
        igImage(mfRenderTargetGetColorAttachmentImTexID(state->renderTarget), (ImVec2){mfRenderTargetGetWidth(state->renderTarget), mfRenderTargetGetHeight(state->renderTarget)}, (ImVec2){0, 0}, (ImVec2){1, 1});
        igEnd();
    }

    // Perf window
    {
        igBegin("Performance", mfnull, ImGuiWindowFlags_None);
        
        igText("Delta time :- %.3fms", mfRendererGetDeltaTime(appState->renderer));
        igText("FPS :- %.0f", (f64)(1000.0/mfRendererGetDeltaTime(appState->renderer)));

        igEnd();
    }

    // Settings window
    {
        igBegin("Settings", mfnull, ImGuiWindowFlags_None);

        bool enableRenderTarget = state->enableRenderTarget;
        igCheckbox("Render to ImGui window", &enableRenderTarget);
        if(enableRenderTarget != state->enableRenderTarget) {
            state->enableRenderTarget = enableRenderTarget;
        }

        igDummy((ImVec2){ 0.0f, 50.0f });

        if(igCollapsingHeader_BoolPtr("Light settings", mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
            f32 posData[3] = {0};
            mfCopyVec3ToFloatArr(posData, state->lightData.lightPos);

            f32 colorData[3] = {0};
            mfCopyVec3ToFloatArr(colorData, state->lightData.lightColor);

            igDragFloat3("LightPos", posData, 0.1f, -5000.0f, 5000.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igDragFloat("Light Intensity", &state->lightData.lightIntensity, 0.5f, 1.0f, 10000.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igColorEdit3("Light Color", colorData, ImGuiColorEditFlags_None);

            bool useNormalMap = state->lightData.useNormalMap;
            bool useAcesTonemapping = state->lightData.useAcesTonemapping;

            igCheckbox("Use normal map", &useNormalMap);
            igCheckbox("Use ACES Tonemapping (Default tonemapper is Reinhard)", &useAcesTonemapping);

            state->lightData.useAcesTonemapping = useAcesTonemapping;
            state->lightData.useNormalMap = useNormalMap;
            state->lightData.lightPos = mfFloatArrToVec3(posData);
            state->lightData.lightColor = mfFloatArrToVec3(colorData);
        }

        igDummy((ImVec2){ 0.0f, 50.0f });

        if(igCollapsingHeader_BoolPtr("Model transform settings", mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
            for(u64 i = 0; i < state->entityCount; i++) {
                igPushID_Int((int)i);
                char name[50];
                sprintf(name, "Entity #%d ###%d", i, i);
                if(igCollapsingHeader_BoolPtr(name, mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
                    MFTransformComponent* transformComponent = mfSceneEntityGetTransformComponent(&state->scene, &state->entities[i]);

                    f32 scale[3] = {0};
                    mfCopyVec3ToFloatArr(scale, transformComponent->scale);
                    f32 position[3] = {0};
                    mfCopyVec3ToFloatArr(position, transformComponent->position);
                    f32 rotation[3] = {0};
                    mfCopyVec3ToFloatArr(rotation, transformComponent->rotationXYZ);
                    
                    igDragFloat3("Postion", position, 0.1f, -1e6f, 1e6f, mfnull, ImGuiSliderFlags_None);
                    igDragFloat3("Scale", scale, 0.1f, 1.0f, 1e6f, mfnull, ImGuiSliderFlags_None);
                    igDragFloat3("Rotation (In degrees)", rotation, 0.1f, 360 * -1e5f, 360 * 1e5f, mfnull, ImGuiSliderFlags_None);
                
                    transformComponent->position = mfFloatArrToVec3(position);
                    transformComponent->scale = mfFloatArrToVec3(scale);
                    transformComponent->rotationXYZ = mfFloatArrToVec3(rotation);
                }
                igPopID();
            }
        }

        igEnd();
    }

    MF_PROFILE_ZONE_END(__temp);
}

void MFTOnUpdate(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest update");

    MFDefaultAppState* appState = (MFDefaultAppState*)pappState;
    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* winConfig = mfWindowGetConfig(appState->window);

    if(state->enableRenderTarget) {
        state->scene.camera.width = state->sceneViewport.x;
        state->scene.camera.height = state->sceneViewport.y;
    } else {
        state->scene.camera.width = winConfig->width;
        state->scene.camera.height = winConfig->height;
    }
    state->scene.camera.update(&state->scene.camera, mfRendererGetDeltaTime(appState->renderer), mfnull);

    state->cameraUboData.proj = state->scene.camera.proj;
    state->cameraUboData.view = state->scene.camera.view;
    state->lightData.camPos = state->scene.camera.pos;
    mfGpuBufferUploadData(state->lightUbo, &state->lightData);
    mfGpuBufferUploadData(state->cameraUbo, &state->cameraUboData);

    if(mfInputIsKeyPressed(appState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(appState->window);
    }
    
    MF_PROFILE_ZONE_END(__temp);
}

#pragma endregion
