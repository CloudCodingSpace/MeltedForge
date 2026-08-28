#pragma once

#include <mf.h>

#include <stb/stb_image.h>

typedef struct CameraUBOData_s {
    MFMat4 prevViewProj;
    MFMat4 viewProj;
} CameraUBOData;

typedef struct PushConstantData_s {
    MFMat4 model;
    MFMat3 normalMat;
} PushConstantData;

typedef struct FSPushConstantData_s {
    int showDepthAttachment;
    int showChromaticAberration;
    float zNear;
    float zFar;
} FSPushConstantData;

typedef struct LightUBOData_s {
    MFVec3 lightPos;
    MFVec3 camPos;
    MFVec3 lightColor;
    f32 lightIntensity;
    f32 iblDiffuseStrength;
    f32 iblSpecularStrength;
    int useNormalMap;
    int useAoMap;
    int useIBL;
} LightUBOData;

typedef struct MFTState_s {
    SLogger logger;

    MFResourceSetLayout* matLayout, *skyboxLayout, *camLightLayout;
    MFResourceSet* skyboxSet, *cameraSet;

    MFPipeline* fsPipeline, *scenePipeline, *depthPrePassPipeline;
    MFGpuBuffer* cameraUbo;
    MFGpuBuffer* lightUbo;
    MFSkybox* skybox;

    MFScene scene;
    u64 entityCount;
    u64* entities;
    MFArray* materialImages;
    
    LightUBOData lightData;
    CameraUBOData cameraUboData;
    FSPushConstantData fsPcData;
    
    ImVec2 sceneViewport;
    MFRenderGraph* renderGraph;
    bool takeScreenshot, enableFustrumCulling;

    MFWindow* window;
    void* renderer;
} MFTState;

void MFTOnInit(void* pstate, void* pappState);
void MFTOnDeinit(void* pstate, void* pappState);
void MFTOnRender(void* pstate, void* pappState);
void MFTOnUIRender(void* pstate, void* pappState);
void MFTOnUpdate(void* pstate, void* pappState);
