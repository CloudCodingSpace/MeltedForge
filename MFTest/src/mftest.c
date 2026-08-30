#include "mftest.h"
#include "core/mfutils.h"
#include "util.h"

#include <stb/stb_image_write.h>

#define INFO(logger, msg, ...) slogLogMsg(logger, SLOG_SEVERITY_INFO, msg, ##__VA_ARGS__)

#pragma region callbacks

static void ResizeCallback(void* pstate) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "Resize callback");

    MFTState* state = (MFTState*)pstate;
    const MFRenderGraphConfig* config = mfRenderGraphGetConfig(state->renderGraph);

    state->scene.camera.width = config->width;
    state->scene.camera.height = config->height;

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
    mfPipelinePushConstant(pipeline, MF_SHADER_STAGE_VERTEX | MF_SHADER_STAGE_FRAGMENT, 0, sizeof(PushConstantData), &modelData);

    MF_PROFILE_ZONE_END(__temp);
}

static void MeshCallbackDepthPrePass(void* _state, MFMat4 transform, const MFMeshComponent* component, u64 meshIdx, MFPipeline* pipeline) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest Mesh callback");
    MFTState* state = (MFTState*)_state;

    PushConstantData modelData = {
        .model = transform
    };

    mfPipelinePushConstant(pipeline, MF_SHADER_STAGE_VERTEX | MF_SHADER_STAGE_FRAGMENT, 0, sizeof(PushConstantData), &modelData);

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
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest scene pipeline bind callback");

    MFTState* state = (MFTState*)_state;

    MFResourceSet* sets[] = {
        state->cameraSet,
        state->skyboxSet
    };

    mfResourceSetsBind(0, MF_ARRAYLEN(sets), sets, pipeline);

    MF_PROFILE_ZONE_END(__temp);
}

static void PipelineBindCallbackDepthPrePass(void* _state, MFPipeline* pipeline) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest scene pipeline bind callback");

    MFTState* state = (MFTState*)_state;

    MFResourceSet* sets[] = {
        state->cameraSet
    };

    mfResourceSetsBind(0, MF_ARRAYLEN(sets), sets, pipeline);

    MF_PROFILE_ZONE_END(__temp);
}

static void DepthPrePass(void* pUserState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest Depth Pre Pass callback");

    MFTState* state = (MFTState*)pUserState;

    MFSceneRenderConfig config = {
        .state = state,
        .entityPipeline = state->depthPrePassPipeline,
        .scissor = mfRendererGetScissor(state->renderer),
        .viewport = mfRendererGetViewport(state->renderer),
        .perMeshDrawCallback = &MeshCallbackDepthPrePass,
        .computeModelMatrix = &ComputeModelMatrix,
        .pipelineBindCallback = &PipelineBindCallbackDepthPrePass,
        .enableFustrumCulling = state->enableFustrumCulling
    };

    mfSceneRender(&state->scene, &config);
    mfSkyboxRender(state->skybox, state->scene.camera.proj, state->scene.camera.view, mfMat4Identity(), MF_SKYBOX_TYPE_NORMAL);

    MF_PROFILE_ZONE_END(__temp);
}

static void ScenePass(void* pUserState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest Scene Pass callback");

    MFTState* state = (MFTState*)pUserState;

    MFSceneRenderConfig config = {
        .state = state,
        .entityPipeline = state->scenePipeline,
        .scissor = mfRendererGetScissor(state->renderer),
        .viewport = mfRendererGetViewport(state->renderer),
        .perMeshDrawCallback = &MeshCallback,
        .computeModelMatrix = &ComputeModelMatrix,
        .pipelineBindCallback = &PipelineBindCallback,
        .enableFustrumCulling = state->enableFustrumCulling
    };

    mfSceneRender(&state->scene, &config);
    mfSkyboxRender(state->skybox, state->scene.camera.proj, state->scene.camera.view, mfMat4Identity(), MF_SKYBOX_TYPE_NORMAL);

    MF_PROFILE_ZONE_END(__temp);
}

static void FSPass(void* pUserState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest FS Pass callback");

    MFTState* state = (MFTState*)pUserState;

    mfPipelineBind(state->fsPipeline, mfRendererGetViewport(state->renderer), mfRendererGetScissor(state->renderer));
    mfRenderGraphBindAttachmentsSet(state->renderGraph, 0, state->fsPipeline);
    mfResourceSetsBind(1, 1, &state->cameraSet, state->fsPipeline);

    mfPipelinePushConstant(state->fsPipeline, MF_SHADER_STAGE_FRAGMENT, 0, sizeof(FSPushConstantData), &state->fsPcData);
    mfRendererDrawVertices(state->renderer, 3, 1, 0, 0);

    MF_PROFILE_ZONE_END(__temp);
}

#pragma endregion

#pragma region Helpers

static void CreateRenderGraph(MFTState* state, MFDefaultAppState* appState) {
    const MFWindowConfig* winConfig = mfWindowGetConfig(appState->window);
    
    MFRenderGraphAttachmentDesc attachments[3] = {};
    // Color attachments
    attachments[0].clearColor = mfRendererGetClearColor(appState->renderer);
    attachments[0].type = MF_RENDER_GRAPH_ATTACHMENT_TYPE_COLOR_ATTACHMENT;
    attachments[0].format = MF_FORMAT_R8G8B8A8_UNORM;
    
    attachments[1].clearColor = mfRendererGetClearColor(appState->renderer);
    attachments[1].type = MF_RENDER_GRAPH_ATTACHMENT_TYPE_COLOR_ATTACHMENT;
    attachments[1].format = MF_FORMAT_R8G8B8A8_UNORM;

    // Final scene depth attachment
    attachments[2].clearColor = mfRendererGetClearColor(appState->renderer);
    attachments[2].type = MF_RENDER_GRAPH_ATTACHMENT_TYPE_DEPTH_STENCIL_ATTACHMENT;
    attachments[2].format = mfRendererGetStandardDepthFormat(appState->renderer);

    u32 outputAttachmentPass2[] = {
        0
    };

    u32 outputAttachmentPass3[] = {
        1
    };

    u32 depthAttachment[] = {
        2
    };

    MFRenderGraphPassDesc passes[3] = {};
    passes[0].name = "Pass #1 - Depth pre pass";
    passes[0].outputColorAttachmentCount = MF_ARRAYLEN(outputAttachmentPass2);
    passes[0].outputColorAttachments = outputAttachmentPass2;
    passes[0].depthStencilAttachment = depthAttachment;
    passes[0].passDrawCallback = &DepthPrePass;
    passes[0].userData = state;
    
    passes[1].name = "Pass #2 - Scene pass";
    passes[1].depthStencilAttachment = depthAttachment;
    passes[1].outputColorAttachmentCount = MF_ARRAYLEN(outputAttachmentPass2);
    passes[1].outputColorAttachments = outputAttachmentPass2;
    passes[1].passDrawCallback = &ScenePass;
    passes[1].userData = state;

    passes[2].name = "Pass #3 - Fullscreen pass";
    passes[2].outputColorAttachmentCount = MF_ARRAYLEN(outputAttachmentPass3);
    passes[2].outputColorAttachments = outputAttachmentPass3;
    passes[2].passDrawCallback = &FSPass;
    passes[2].userData = state;

    MFRenderGraphConfig config = {
        .attachmentCount = MF_ARRAYLEN(attachments),
        .attachments = attachments,
        .passCount = MF_ARRAYLEN(passes),
        .passes = passes,
        .width = state->sceneViewport.x,
        .height = state->sceneViewport.y,
        .resizeCallbackUserState = state,
        .resizeCallback = &ResizeCallback
    };

    state->renderGraph = mfRenderGraphCreate(appState->renderer, config);
}

static void CreatePipeline(MFTState* state) {
    u32 attributeCount = 0, bindingCount = 1;
    const MFWindowConfig* config = mfWindowGetConfig(state->window);
    MFVertexInputAttributeDescription* attributes = getVertAttribDescs(&attributeCount);
    MFVertexInputBindingDescription bindings = getVertBindingDesc();

    MFPushConstantRange range = {
        .offset = 0,
        .size = sizeof(PushConstantData),
        .stage = MF_SHADER_STAGE_VERTEX | MF_SHADER_STAGE_FRAGMENT
    };

    MFPushConstantRange fsRange = {
        .offset = 0,
        .size = sizeof(FSPushConstantData),
        .stage = MF_SHADER_STAGE_FRAGMENT
    };

    MFResourceSetLayout* fsLayouts[] = {
        mfRenderGraphGetAttachmentsSetLayout(state->renderGraph),
        state->camLightLayout
    };

    MFResourceSetLayout* layouts[] = {
        state->camLightLayout,
        state->skyboxLayout,
        state->matLayout
    };

    MFResourceSetLayout* depthPrePassLayouts[] = {
        state->camLightLayout
    };

    // FS Pipeline
    MFPipelineConfig info = {
        .graphicsConfig = {
            .extent = (MFVec2){ .x = config->width, .y = config->height },
            .hasDepth = true,
            .depthCompareOp = MF_COMPARE_OP_DEFAULT,
            .transparent = true,
            .vertPath = "mftshaders/fs.vert.spv",
            .fragPath = "mftshaders/fs.frag.spv",
            .cullMode = MF_CULL_MODE_BACK_BIT,
            .renderGraph = state->renderGraph,
            .renderGraphPassIdx = 2
        },
        .type = MF_PIPELINE_TYPE_GRAPHICS,
        .resourceLayoutCount = MF_ARRAYLEN(fsLayouts),
        .resourceLayouts = fsLayouts,
        .pushConstRangeCount = 1,
        .pushConstRanges = &fsRange
    };

    state->fsPipeline = mfPipelineCreate(state->renderer, info);

    // Scene pipeline
    info.graphicsConfig.depthCompareOp = MF_COMPARE_OP_LESS_OR_EQUAL;
    info.graphicsConfig.renderGraphPassIdx = 1;
    info.graphicsConfig.vertPath = "mftshaders/default.vert.spv";
    info.graphicsConfig.fragPath = "mftshaders/default.frag.spv";
    info.graphicsConfig.attributesCount = attributeCount;
    info.graphicsConfig.attributes = attributes,
    info.graphicsConfig.bindingsCount = bindingCount,
    info.graphicsConfig.bindings = &bindings,
    info.graphicsConfig.cullMode = MF_CULL_MODE_BACK_BIT;
    info.pushConstRangeCount = 1;
    info.pushConstRanges = &range;
    info.resourceLayoutCount = MF_ARRAYLEN(layouts);
    info.resourceLayouts = layouts;

    state->scenePipeline = mfPipelineCreate(state->renderer, info);

    // Depth pre pass pipeline
    info.graphicsConfig.depthCompareOp = MF_COMPARE_OP_DEFAULT;
    info.graphicsConfig.renderGraphPassIdx = 0;
    info.graphicsConfig.vertPath = "mftshaders/depthprepass.vert.spv";
    info.graphicsConfig.fragPath = "mftshaders/depthprepass.frag.spv";
    info.graphicsConfig.attributesCount = attributeCount;
    info.graphicsConfig.attributes = attributes,
    info.graphicsConfig.bindingsCount = bindingCount,
    info.graphicsConfig.bindings = &bindings,
    info.graphicsConfig.cullMode = MF_CULL_MODE_BACK_BIT;
    info.pushConstRangeCount = 1;
    info.pushConstRanges = &range;
    info.resourceLayoutCount = MF_ARRAYLEN(depthPrePassLayouts);
    info.resourceLayouts = depthPrePassLayouts;

    state->depthPrePassPipeline = mfPipelineCreate(state->renderer, info);

    MF_FREEMEM(attributes);
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
            
            state->matLayout = mfResourceSetLayoutCreate(MF_ARRAYLEN(bindings), bindings, appState->renderer);
        }
    
        // Skybox layout
        {
            MFResourceSetBindings bindings[] = {
                { mfGpuImageGetDescription(skyboxImage),    0 },
                { mfGpuImageGetDescription(irradianceMap),  1 },
                { mfGpuImageGetDescription(prefilteredMap), 2 },
                { mfGpuImageGetDescription(brdfLut),        3 }
            };
            state->skyboxLayout = mfResourceSetLayoutCreate(MF_ARRAYLEN(bindings), bindings, state->renderer);
        }
        
        // Camera layout
        {
            MFResourceSetBindings bindings[] = {
                { mfGpuBufferGetDescription(state->cameraUbo), 0 },
                { mfGpuBufferGetDescription(state->lightUbo),  1 }
            };
            state->camLightLayout = mfResourceSetLayoutCreate(MF_ARRAYLEN(bindings), bindings, state->renderer);
        }
    }
    // Resource sets
    {
        // Mesh sets
        for(u64 k = 0; k < state->scene.meshCompPool.len; k++) {
            MFMeshComponent* component = &mfArrayGetElement(state->scene.meshCompPool, MFMeshComponent, k);
            for(u64 i = 0; i < component->model.meshCount; i++) {
                MFResourceSet* set = mfResourceSetCreate(state->matLayout, appState->renderer);

                MFGpuImage* diffuseImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_DIFFUSE, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* normalImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_NORMAL, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* metallicRoughnessImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_METALLIC_ROUGHNESS, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* emissiveImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_EMISSIVE, &state->materialImages[k], &component->model, i, appState->renderer);
                MFGpuImage* aoImage = mfMaterialSystemGetImageFromArray(MF_MODEL_MAT_TEXTURE_AO, &state->materialImages[k], &component->model, i, appState->renderer);

                MFGpuImage* images[] = {
                    diffuseImage,
                    normalImage,
                    metallicRoughnessImage,
                    emissiveImage,
                    aoImage
                };

                mfResourceSetUpdate(set, MF_ARRAYLEN(images), images, 0, mfnull);

                component->model.meshes[i].mat.set = set;
            }
        }
        
        // Camera set
        {
            MFGpuBuffer* buffers[] = {
                state->cameraUbo,
                state->lightUbo
            };

            state->cameraSet = mfResourceSetCreate(state->camLightLayout, appState->renderer);
            mfResourceSetUpdate(state->cameraSet, 0, mfnull, MF_ARRAYLEN(buffers), buffers);            
        }

        // Skybox set
        {
            state->skyboxSet = mfResourceSetCreate(state->skyboxLayout, state->renderer);

            MFGpuImage* images[] = {
                skyboxImage,
                irradianceMap,
                prefilteredMap,
                brdfLut
            };

            mfResourceSetUpdate(state->skyboxSet, MF_ARRAYLEN(images), images, 0, mfnull);
        }

        for(u64 i = 0; i < state->scene.meshCompPool.len; i++) {
            mfMaterialSystemDestroyModelMatImages(&state->materialImages[i]);
        }
    }
}

static void CreateUBOs(MFTState* state, MFDefaultAppState* appState) {
    MFGpuBufferConfig config = {
        .type = MF_GPU_BUFFER_TYPE_UBO,
        .size = sizeof(CameraUBOData),
        .stage = MF_SHADER_STAGE_VERTEX | MF_SHADER_STAGE_FRAGMENT,
        .frequentUpdates = true,
        .frameSynced = true
    };

    state->cameraUboData.viewProj = mfMat4Mul(state->scene.camera.proj, state->scene.camera.view);

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
    state->materialImages = MF_ALLOCMEM(MFArray, sizeof(MFArray) * state->scene.meshCompPool.len);
    for(u64 i = 0; i < state->scene.meshCompPool.len; i++) {
        MFMeshComponent* component = &mfArrayGetElement(state->scene.meshCompPool, MFMeshComponent, i);
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
    
    state->fsPcData.zNear = camera.nearPlane;
    state->fsPcData.zFar = camera.farPlane;

    mfSceneCreate(&state->scene, camera, &vertBuilder, appState->renderer);
    if(!mfSceneDeserialize(&state->scene, "./mftscene.bin")) {
        state->entityCount = 1000;
        state->entities = MF_ALLOCMEM(u64, sizeof(u64) * state->entityCount);

        MFMeshComponent mComp = {
            // .path = "mftmeshes/DeccerCubes/SM_Deccer_Cubes_Textured_Complex.gltf",
            // .path = "mftmeshes/DeccerCubes/SM_Deccer_Cubes.gltf",
            .path = "mftmeshes/Damaged Helmet/DamagedHelmet.gltf",
            // .path = "mftmeshes/Sponza/glTF/Sponza.gltf",
            // .path = "mftmeshes/pistol/service_pistol.gltf",
            // .path = "mftmeshes/sofa/sofa_1k.gltf",
            .perVertSize = sizeof(Vertex)
        };

        mfSceneAddMeshComponent(&state->scene, &mComp);

        for(u32 i = 0; i < state->entityCount; i++) {
            MFTransformComponent tComp = {
                .position = (MFVec3){mfRandomFloatMinMax(-100, 100), mfRandomFloatMinMax(-100, 100), mfRandomFloatMinMax(-100, 100)},
                .rotationXYZ = (MFVec3){mfRandomFloatMinMax(-100, 100), mfRandomFloatMinMax(-100, 100), mfRandomFloatMinMax(-100, 100)},
                .scale = (MFVec3){2, 2, 2}
            };
            mfSceneAddTransformComponent(&state->scene, &tComp);
        
            state->entities[i] = mfSceneCreateEntity(&state->scene);
            mfSceneEntityAttachMeshComponent(&state->scene, &state->entities[i], &mComp);
            mfSceneEntityAttachTransformComponent(&state->scene, &state->entities[i], &tComp);
        }
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

    state->renderer = appState->renderer;
    state->window = appState->window;
    state->enableFustrumCulling = true;
    state->sceneViewport = (ImVec2){ winConfig->width, winConfig->height };
    state->fsPcData = (FSPushConstantData) {
        .zNear = 0.01f,
        .zFar = 1000.0f
    };
   
    slogLoggerCreate(&state->logger, "MFTest", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE);
    slogLoggerSetName(&state->logger, "MFTest");

    mfRendererSetClearColor(appState->renderer, mfVec3Create(0, 0, 0.01f));
    mfRendererSetResizeCallback(appState->renderer, state, &ResizeCallback);

    // Printing available optional render features
    {
        MFOptionalRenderFeatures flags = mfRendererGetSupportedOptionalRenderFeatures(state->renderer);
        if(mfFlagContainsBits(flags, MF_OPTIONAL_RENDER_FEATURE_SCALAR_LAYOUT))
            slogLogMsg(&state->logger, SLOG_SEVERITY_INFO, "Scalar layout feature supported!");
        else {
            slogLogMsg(&state->logger, SLOG_SEVERITY_FATAL, "Scalar layout feature is necessary to run MFTest but it isn't available!");
            exit(-1);
        }

        if(mfFlagContainsBits(flags, MF_OPTIONAL_RENDER_FEATURE_BUFFER_DEVICE_ADDRESS))
            slogLogMsg(&state->logger, SLOG_SEVERITY_INFO, "Buffer device addresses feature supported!");
        if(mfFlagContainsBits(flags, MF_OPTIONAL_RENDER_FEATURE_DESCRIPTOR_INDEXING))
            slogLogMsg(&state->logger, SLOG_SEVERITY_INFO, "Deascriptor indexing feature supported!");
        if(mfFlagContainsBits(flags, MF_OPTIONAL_RENDER_FEATURE_SAMPLER_ANISOTROPY))
            slogLogMsg(&state->logger, SLOG_SEVERITY_INFO, "Sampler anisotropy feature supported!");
        if(mfFlagContainsBits(flags, MF_OPTIONAL_RENDER_FEATURE_SHADER_NON_UNIFORM_ACCESS))
            slogLogMsg(&state->logger, SLOG_SEVERITY_INFO, "Non uniform access feature supported!");
        if(mfFlagContainsBits(flags, MF_OPTIONAL_RENDER_FEATURE_VARIABLE_DESCRIPTOR_SIZES))
            slogLogMsg(&state->logger, SLOG_SEVERITY_INFO, "Variable descriptor sizes feature supported!");
    }

    CreateRenderGraph(state, appState);

    // Skybox
    {
        MFSkyboxConfig config = {
            .faceSize = 512,
            .environmentPath = "mftskyboxes/3.hdr",
            .generatePbrMaps = true,
            .renderGraph = state->renderGraph,
            .renderGraphPassIdx = 1
        };
        state->skybox = mfSkyboxCreate(config, appState->renderer);
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

    mfSceneSerialize(&state->scene, "./mftscene.bin");
    for(u64 i = 0; i < state->entityCount; i++)
        mfSceneDeleteEntity(&state->scene, &state->entities[i]);
    mfSceneDestroy(&state->scene);

    mfResourceSetDestroy(&state->cameraSet);
    mfResourceSetDestroy(&state->skyboxSet);

    mfResourceSetLayoutDestroy(&state->matLayout);
    mfResourceSetLayoutDestroy(&state->skyboxLayout);
    mfResourceSetLayoutDestroy(&state->camLightLayout);
    
    mfGpuBufferFree(&state->cameraUbo);
    mfGpuBufferFree(&state->lightUbo);

    mfSkyboxDestroy(&state->skybox);

    mfRenderGraphDestroy(&state->renderGraph);

    mfPipelineDestroy(&state->fsPipeline);
    mfPipelineDestroy(&state->scenePipeline);
    mfPipelineDestroy(&state->depthPrePassPipeline);

    MF_FREEMEM(state->entities);
    MF_FREEMEM(state->materialImages);
}

void MFTOnRender(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest render");

    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;
    const MFWindowConfig* winConfig = mfWindowGetConfig(appState->window);

    const MFRenderGraphConfig* rgConfig = mfRenderGraphGetConfig(state->renderGraph);
    if((rgConfig->width != state->sceneViewport.x) || (rgConfig->height != state->sceneViewport.y)) {
        mfRenderGraphResize(state->renderGraph, state->sceneViewport.x, state->sceneViewport.y);
    }

    mfRenderGraphInvoke(state->renderGraph, false);

    MF_PROFILE_ZONE_END(__temp);
}

void MFTOnUIRender(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest update");

    MFTState* state = (MFTState*)pstate;
    MFDefaultAppState* appState = (MFDefaultAppState*) pappState;

    // Scene
    {
        igDockSpaceOverViewport(igGetID_Str("Dockspace"), igGetMainViewport(), ImGuiDockNodeFlags_None, mfnull);
        const MFRenderGraphConfig* config = mfRenderGraphGetConfig(state->renderGraph);

        igBegin("Scene", mfnull, ImGuiWindowFlags_NoScrollbar);
        igGetContentRegionAvail(&state->sceneViewport);
        igImage(mfRenderGraphGetAttachmentImTextureID(state->renderGraph, 1), (ImVec2){ config->width, config->height }, (ImVec2){ 0, 0 }, (ImVec2){ 1, 1 });
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

        if(igButton("Take screenshot", (ImVec2){200, 35})) {
            state->takeScreenshot = true;
        }

        igDummy((ImVec2){ 0.0f, 20.0f });

        // Camera settings
        if(igCollapsingHeader_BoolPtr("Camera effects", mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
            bool showDepthAttachment = state->fsPcData.showDepthAttachment;
            bool showChromaticAberration = state->fsPcData.showChromaticAberration;

            igCheckbox("Show depth attachment", &showDepthAttachment);
            igCheckbox("Enable fustrum culling", &state->enableFustrumCulling);
            igCheckbox("Enable chromatic aberration", &showChromaticAberration);
        
            state->fsPcData.showDepthAttachment = showDepthAttachment;
            state->fsPcData.showChromaticAberration = showChromaticAberration;
        }

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

        /*
        if(igCollapsingHeader_BoolPtr("Model transform settings", mfnull, ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
            for(u64 i = 0; i < state->entityCount; i++) {
                igPushID_Int((int)i);
                char name[50];
                sprintf(name, "Entity #%zu ###%zu", i, i);
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
        */

        igEnd();
    }

    MF_PROFILE_ZONE_END(__temp);
}

void MFTOnUpdate(void* pstate, void* pappState) {
    MF_PROFILE_ZONE_START_NAMED(__temp, "MFTest update");

    MFDefaultAppState* appState = (MFDefaultAppState*)pappState;
    MFTState* state = (MFTState*)pstate;
    const MFWindowConfig* winConfig = mfWindowGetConfig(appState->window);
    const MFRenderGraphConfig* rgConfig = mfRenderGraphGetConfig(state->renderGraph);

    if(state->takeScreenshot) {
        u32 width, height;
        u8* pixels = mfRenderGraphGetAttachmentPixels(state->renderGraph, 1, &width, &height);
        u32 bytesPerPixel =  mfRenderGraphGetAttachmentBytesPerPixel(state->renderGraph, 1);

        stbi_write_png("mftscreenshot.png", width, height, bytesPerPixel, pixels, width * bytesPerPixel * sizeof(u8));

        MF_FREEMEM(pixels);
        state->takeScreenshot = false;
    }

    state->scene.camera.width = rgConfig->width;
    state->scene.camera.height = rgConfig->height;

    state->scene.camera.update(&state->scene.camera, mfRendererGetDeltaTime(appState->renderer), mfnull);

    state->cameraUboData.prevViewProj = state->cameraUboData.viewProj;
    state->cameraUboData.viewProj = mfMat4Mul(state->scene.camera.proj, state->scene.camera.view);
    state->lightData.camPos = state->scene.camera.pos;

    mfGpuBufferUploadData(state->lightUbo, &state->lightData);
    mfGpuBufferUploadData(state->cameraUbo, &state->cameraUboData);

    if(mfInputIsKeyPressed(appState->window, MF_KEY_B)) {
        u32 width, height;
        u8* pixels = mfRendererGetCurrentImagePixels(appState->renderer, &width, &height);
        u32 bytesPerPixel =  mfRendererGetImageBytesPerPixel(appState->renderer);

        stbi_write_png("mftscreenshot.png", width, height, bytesPerPixel, pixels, width * bytesPerPixel * sizeof(u8));

        MF_FREEMEM(pixels);
    }

    if(mfInputIsKeyPressed(appState->window, MF_KEY_ESCAPE)) {
        mfWindowClose(appState->window);
    }
    
    MF_PROFILE_ZONE_END(__temp);
}

#pragma endregion
