#pragma once

#include <mf.h>

#include <stb/stb_image.h>

typedef struct UBOData_s {
    MFMat4 proj;
    MFMat4 view;
} UBOData;

typedef struct PushConstantData_s {
    MFMat4 model;
    MFMat3 normalMat;
} PushConstantData;

typedef struct LightUBOData_s {
    MFVec3 lightPos;
    MFVec3 camPos;
    MFVec3 lightColor;
    f32 ambientFactor;
    f32 specularFactor;
    f32 lightIntensity;
    int isPoint;
    int useNormalMap;
    int showGlassMat;
} LightUBOData;

typedef struct MFTState_s {
    SLogger logger;

    MFResourceSetLayout* layout, *layout2;
    u64 setCount;
    MFResourceSet** sets;
    MFResourceSet* set2;

    MFPipeline* pipeline;
    MFPipeline* pipeline2;
    MFGpuBuffer* cameraUbo;
    MFGpuBuffer* lightUbo;
    MFSkybox* skybox;
    MFSkybox* skybox2;

    MFScene scene;
    u64 entity;
    MFArray materialImages;
    
    LightUBOData lightData;
    UBOData cameraUboData;
    
    MFRenderTarget* renderTarget;
    ImVec2 sceneViewport;
    bool enableRenderTarget, showIrradiance;
    
    MFWindow* window;
    void* renderer;
} MFTState;

void MFTOnInit(void* pstate, void* pappState);
void MFTOnDeinit(void* pstate, void* pappState);
void MFTOnRender(void* pstate, void* pappState);
void MFTOnUIRender(void* pstate, void* pappState);
void MFTOnUpdate(void* pstate, void* pappState);
