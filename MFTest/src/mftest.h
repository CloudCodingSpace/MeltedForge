#pragma once

#include <mf.h>

#include <stb/stb_image.h>

typedef struct UBOData_s {
    MFMat4 prevProj;
    MFMat4 prevView;
    MFMat4 proj;
    MFMat4 view;
} UBOData;

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

    MFResourceSetLayout* layout, *skyboxLayout, *cameraLayout;
    MFResourceSet* skyboxSet, *cameraSet;

    MFPipeline* fsPipeline, *rtPipeline;
    MFGpuBuffer* cameraUbo;
    MFGpuBuffer* lightUbo;
    MFSkybox* skybox;

    MFScene scene;
    u64 entityCount;
    u64* entities;
    MFArray* materialImages;
    
    LightUBOData lightData;
    UBOData cameraUboData;
    MFMat4 prevProj, prevView;
    
    MFRenderTarget* renderTarget;
    bool takeScreenshot, showDepthAttachment, showColorAberration;
    
    MFWindow* window;
    void* renderer;
} MFTState;

void MFTOnInit(void* pstate, void* pappState);
void MFTOnDeinit(void* pstate, void* pappState);
void MFTOnRender(void* pstate, void* pappState);
void MFTOnUIRender(void* pstate, void* pappState);
void MFTOnUpdate(void* pstate, void* pappState);
