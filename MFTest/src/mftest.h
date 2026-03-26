#pragma once

#include <mf.h>

#include <stb/stb_image.h>

typedef struct UBOData_s {
    MFMat4 proj;
    MFMat4 view;
} UBOData;

typedef struct PushConstantData_s {
    MFMat4 model;
    MFMat4 normalMat;
} PushConstantData;

typedef struct LightUBOData_s {
    MFVec3 lightPos;
    f32 ambientFactor;
    MFVec3 camPos;
    f32 specularFactor;
    MFVec3 lightColor;
    float lightIntensity;
} LightUBOData;

typedef struct MFTState_s {
    SLogger logger;

    MFResourceSetLayout* layout;
    u64 setCount;
    MFResourceSet** sets;

    MFPipeline* pipeline;
    MFGpuBuffer* cameraUbo;
    MFGpuBuffer* lightUbo;

    MFScene scene;
    MFEntity* entity;
    MFArray materialImages;
    
    MFCamera camera;
    LightUBOData lightData;
    UBOData cameraUboData;
    
    MFRenderTarget* renderTarget;
    ImVec2 sceneViewport;
    
    void* renderer;
} MFTState;

void MFTOnInit(void* pstate, void* pappState);
void MFTOnDeinit(void* pstate, void* pappState);
void MFTOnRender(void* pstate, void* pappState);
void MFTOnUIRender(void* pstate, void* pappState);
void MFTOnUpdate(void* pstate, void* pappState);
