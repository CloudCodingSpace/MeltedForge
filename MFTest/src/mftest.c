#include "mftest.h"
#include "util.h"

#include <stb/stb_image_write.h>

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
        state->cameraLayout,
        state->skyboxLayout,
        state->layout
    };

    MFPipelineConfig info = {
        .graphicsConfig = {
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
            .cullMode = MF_CULL_MODE_BACK_BIT
        },
        .resourceLayoutCount = MF_ARRAYLEN(layouts),
        .resourceLayouts = layouts,
        .pushConstRangeCount = 1,
        .pushConstRanges = &range,
        .type = MF_PIPELINE_TYPE_GRAPHICS
    };

    state->pipeline = mfPipelineCreate(state->renderer, info);

    info.graphicsConfig.extent = (MFVec2){ .x = state->sceneViewport.x, .y = state->sceneViewport.y };
    info.graphicsConfig.renderTarget = state->renderTarget;

    state->rtPipeline = mfPipelineCreate(state->renderer, info);

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
    mfResourceSetsBind(2, 1, set, pipeline);
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

    MFResourceSet* sets[] = {
        state->cameraSet,
        state->skyboxSet
    };

    mfResourceSetsBind(0, MF_ARRAYLEN(sets), sets, pipeline);
}

static void CreateResourceHandles(MFTState* state, MFDefaultAppState* appState) {
    MFGpuImage* skyboxImage = mfSkyboxGetCubemapImage(state->skybox);
    MFGpuImage* irradianceMap = mfSkyboxGetIrradianceCubemapImage(state->skybox);
    MFGpuImage* prefilteredMap = mfSkyboxGetPrefilteredCubemapImage(state->skybox);
    MFGpuImage* brdfLut = mfSkyboxGetBRDFLUT(state->skybox);
    // Resource layouts
    {
        // Mesh set layout
        {
            u32 totalMeshCount = 0;
            for(u32 i = 0; i < state->entityCount; i++) {
                MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entities[i]);
                totalMeshCount += component->model.meshCount;
            }
            MFMeshComponent* component = mfSceneEntityGetMeshComponent(&state->scene, &state->entities[0]);
            MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages[0], &component->model, 0, appState->renderer);
            MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages[0], &component->model, 0, appState->renderer);
            MFGpuImage* metallicRoughnessImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_METALLIC_ROUGHNESS, &state->materialImages[0], &component->model, 0, appState->renderer);
            MFGpuImage* emissiveImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_EMISSIVE, &state->materialImages[0], &component->model, 0, appState->renderer);
            MFGpuImage* aoImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_AO, &state->materialImages[0], &component->model, 0, appState->renderer);

            MFResourceSetBindings bindings[] = {
                { mfGpuImageGetDescription(diffuseImage), 0 }, // NOTE: Description for one image is enough since they have the same bindings 
                { mfGpuImageGetDescription(normalImage), 1 }, // NOTE: Description for one image is enough since they have the same bindings 
                { mfGpuImageGetDescription(metallicRoughnessImage), 2 }, // NOTE: Description for one image is enough since they have the same bindings 
                { mfGpuImageGetDescription(emissiveImage), 3 }, // NOTE: Description for one image is enough since they have the same bindings 
                { mfGpuImageGetDescription(aoImage), 4 }, // NOTE: Description for one image is enough since they have the same bindings
            };
            
            state->layout = mfResourceSetLayoutCreate(MF_ARRAYLEN(bindings), bindings, totalMeshCount, appState->renderer);
        }
    
        // Skybox layout
        {
            MFResourceSetBindings bindings[] = {
                { mfGpuImageGetDescription(skyboxImage),    0 },
                { mfGpuImageGetDescription(irradianceMap),  1 },
                { mfGpuImageGetDescription(prefilteredMap), 2 },
                { mfGpuImageGetDescription(brdfLut),        3 }
            };
            state->skyboxLayout = mfResourceSetLayoutCreate(MF_ARRAYLEN(bindings), bindings, 1, state->renderer);
        }
        
        // Camera layout
        {
            MFResourceSetBindings bindings[] = {
                { mfGpuBufferGetDescription(state->cameraUbo), 0 },
                { mfGpuBufferGetDescription(state->lightUbo),  1 }
            };
            state->cameraLayout = mfResourceSetLayoutCreate(MF_ARRAYLEN(bindings), bindings, 1, state->renderer);
        }
    }
    // Resource sets
    {
        // Mesh sets
        for(u64 k = 0; k < state->scene.meshCompPool.len; k++) {
            MFMeshComponent* component = &mfArrayGetElement(state->scene.meshCompPool, MFMeshComponent, k);
            for(u64 i = 0; i < component->model.meshCount; i++) {
                MFResourceSet* set = mfResourceSetCreate(state->layout, appState->renderer);

                MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* metallicRoughnessImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_METALLIC_ROUGHNESS, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* emissiveImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_EMISSIVE, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* aoImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_AO, &state->materialImages[k], &component->model, i, appState->renderer);

                MFArray images = mfArrayCreate(2, sizeof(MFGpuImage*));
                mfArrayAddElement(&images, MFGpuImage*, diffuseImage);
                mfArrayAddElement(&images, MFGpuImage*, normalImage);
                mfArrayAddElement(&images, MFGpuImage*, metallicRoughnessImage);
                mfArrayAddElement(&images, MFGpuImage*, emissiveImage);
                mfArrayAddElement(&images, MFGpuImage*, aoImage);

                mfResourceSetUpdate(set, &images, mfnull);

                mfArrayDestroy(&images);
                component->model.meshes[i].mat.set = set;
            }
        }
        
        // Camera set
        {
            MFArray buffers = mfArrayCreate(2, sizeof(MFGpuBuffer*));
            mfArrayAddElement(&buffers, MFGpuBuffer*, state->cameraUbo);
            mfArrayAddElement(&buffers, MFGpuBuffer*, state->lightUbo);
            
            state->cameraSet = mfResourceSetCreate(state->cameraLayout, appState->renderer);
            mfResourceSetUpdate(state->cameraSet, mfnull, &buffers);
            
            mfArrayDestroy(&buffers);
        }

        // Skybox set
        {
            state->skyboxSet = mfResourceSetCreate(state->skyboxLayout, state->renderer);

            MFArray images = mfArrayCreate(1, sizeof(MFGpuImage*));
            mfArrayAddElement(&images, MFGpuImage*, skyboxImage);
            mfArrayAddElement(&images, MFGpuImage*, irradianceMap);
            mfArrayAddElement(&images, MFGpuImage*, prefilteredMap);
            mfArrayAddElement(&images, MFGpuImage*, brdfLut);
            mfResourceSetUpdate(state->skyboxSet, &images, mfnull);
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
        .stage = MF_SHADER_STAGE_VERTEX,
        .frequentUpdates = true,
        .frameSynced = true
    };

    state->cameraUboData.proj = state->scene.camera.proj;
    state->cameraUboData.view = state->scene.camera.view;

    state->cameraUbo = mfGpuBufferAllocate(config, appState->renderer);
    mfGpuBufferUploadData(state->cameraUbo, &state->cameraUboData);
    
    config.size = sizeof(LightUBOData);
    config.stage = MF_SHADER_STAGE_FRAGMENT;
    
    state->lightData = (LightUBOData) {
        .camPos = state->scene.camera.pos,
        .lightPos = (MFVec3){0.0f, 20.0f, 10.0f},
        .lightColor = (MFVec3){1.0f, 1.0f, 1.0f},
        .lightIntensity = 100,
        .iblDiffuseStrength = 1,
        .iblSpecularStrength = 1,
        .useNormalMap = true,
        .useAoMap = true,
        .useIBL = true
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

        MF_FREEMEM(basePath);
    }
}

static void CreateScene(MFTState* state, MFDefaultAppState* appState) {
    MFCamera camera = {};
    mfCameraCreate(&camera, appState->window, mfWindowGetConfig(appState->window)->width, mfWindowGetConfig(appState->window)->height, 60, 0.01f, 1000.0f, 0.025f, 0.075f, (MFVec3){0.0f, 0.0f, 2.0f});
    mfSceneCreate(&state->scene, camera, &vertBuilder, appState->renderer);
    if(!mfSceneDeserialize(&state->scene, "./mftscene.bin")) {
        state->entityCount = 2;
        state->entities = MF_ALLOCMEM(u64, sizeof(u64) * state->entityCount);
        state->entities[0] = mfSceneCreateEntity(&state->scene);
        state->entities[1] = mfSceneCreateEntity(&state->scene);

        MFMeshComponent mComp = {
            // .path = "mftmeshes/DeccerCubes/SM_Deccer_Cubes_Textured_Complex.gltf",
            // .path = "mftmeshes/DeccerCubes/SM_Deccer_Cubes.gltf",
            .path = "mftmeshes/Damaged Helmet/DamagedHelmet.gltf",
            // .path = "mftmeshes/Sponza/glTF/Sponza.gltf",
            // .path = "mftmeshes/pistol/service_pistol.gltf",
            // .path = "mftmeshes/sofa/sofa_1k.gltf",
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
        
        MFMeshComponent mComp2 = {
            // .path = "mftmeshes/Damaged Helmet/DamagedHelmet.gltf",
            // .path = "mftmeshes/Sponza/glTF/Sponza.gltf",
            // .path = "mftmeshes/pistol/service_pistol.gltf",
            .path = "mftmeshes/sofa/sofa_1k.gltf",
            .perVertSize = sizeof(Vertex)
        };
        mfSceneAddMeshComponent(&state->scene, &mComp2);

        mfSceneEntityAttachMeshComponent(&state->scene, &state->entities[1], &mComp2);
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
            .faceSize = 512,
            .environmentPath = "mftskyboxes/3.hdr",
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
    
    slogLoggerDestroy(&state->logger);

    // Deleting resource sets
    {
        u64 count = 0;
        mfSceneGetValidMeshComponents(&state->scene, &count, mfnull);
        MFMeshComponent* components = MF_ALLOCMEM(MFMeshComponent, sizeof(MFMeshComponent) * count);
        mfSceneGetValidMeshComponents(&state->scene, &count, components);

        for(u32 i = 0; i < count; i++) {
            for(u32 j = 0; j < components[i].model.meshCount; j++) {
                MFMesh* mesh = &components[i].model.meshes[j];
                mfResourceSetDestroy(mesh->mat.set);
            }
        }

        MF_FREEMEM(components);
    }

    mfResourceSetDestroy(state->cameraSet);
    mfResourceSetDestroy(state->skyboxSet);

    mfResourceSetLayoutDestroy(state->layout);
    mfResourceSetLayoutDestroy(state->skyboxLayout);
    mfResourceSetLayoutDestroy(state->cameraLayout);
    
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
    mfPipelineDestroy(state->rtPipeline);

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
        .entityPipeline = (state->enableRenderTarget) ? state->rtPipeline : state->pipeline,
        .scissor = mfRendererGetScissor(state->renderer),
        .viewport = mfRendererGetViewport(state->renderer),
        .perMeshDrawCallback = &MeshCallback,
        .computeModelMatrix = &ComputeModelMatrix,
        .pipelineBindCallback = &PipelineBindCallback
    };

    mfSceneRender(&state->scene, &config);

    if(state->enableRenderTarget) {
        mfSkyboxRender(state->skybox2, state->cameraUboData.proj, state->cameraUboData.view, mfMat4Identity(), MF_SKYBOX_TYPE_NORMAL);
        mfRenderTargetEnd(state->renderTarget, false);
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

        igCheckbox("Render to ImGui window", &state->enableRenderTarget);

        if(igButton("Take screenshot", (ImVec2){200, 35})) {
            state->takeScreenshot = true;
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
            igDragFloat("Diffuse IBL strength", &state->lightData.iblDiffuseStrength, 0.01f, 0.0f, 1.0f, mfnull, ImGuiSliderFlags_ClampOnInput);
            igDragFloat("Specular IBL strength", &state->lightData.iblSpecularStrength, 0.01f, 0.0f, 1.0f, mfnull, ImGuiSliderFlags_ClampOnInput);

            bool useNormalMap = state->lightData.useNormalMap;
            bool useAoMap = state->lightData.useAoMap;
            bool useIBL = state->lightData.useIBL;

            igCheckbox("Use normal map", &useNormalMap);
            igCheckbox("Use AO map", &useAoMap);
            igCheckbox("Use IBL", &useIBL);

            state->lightData.useIBL = useIBL;
            state->lightData.useAoMap = useAoMap;
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

    if(state->takeScreenshot) {
        u32 width, height;
        u8* pixels = mfRenderTargetGetCurrentImagePixels(state->renderTarget, &width, &height);
        u32 bytesPerPixel =  mfRendererGetImageBytesPerPixel(appState->renderer);

        stbi_write_png("mftscreenshot.png", width, height, bytesPerPixel, pixels, width * bytesPerPixel * sizeof(u8));

        MF_FREEMEM(pixels);
        state->takeScreenshot = false;
    }

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
