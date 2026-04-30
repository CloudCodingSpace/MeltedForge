#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "objects/mfmesh.h"
#include "../mfgpu_res.h"
#include "../mfpipeline.h"
#include "../mfskybox.h"

#include "backend.h"

struct MFSkybox_s {
    MFMesh mesh;
    MFPipeline* pipeline;
    MFResourceSet* set;
    MFResourceSet* set2;
    MFResourceSetLayout* layout;

    MFGpuImage* image;
    MFGpuImage* irradiance;
    MFSkyboxConfig config;
    VulkanBackend* backend;
    MFRenderer* renderer;
    bool init, isHdr;
};

void SkyboxConvertEnvMapToSkybox(MFSkybox* skybox, MFSkyboxConfig config, MFRenderer* renderer);
void SkyboxGenerateIrradiance(MFSkybox* skybox, MFSkyboxConfig config, MFRenderer* renderer);

#ifdef __cplusplus
}
#endif