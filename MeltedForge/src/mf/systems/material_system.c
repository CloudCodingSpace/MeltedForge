#include "material_system.h"

#include "renderer/mfimage.h"
#include <stb/stb_image.h>

MFGpuImage* loadImage(const char* path, void* renderer) {
    u8 blackColor[4] = {0x00, 0x00, 0x00, 0xff};
    u32 width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    u8* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- \n");
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, stbi_failure_reason());
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "\n");
        pixels = blackColor;
        width = 1;
        height = 1;
    }

    MFGpuImage* tex = MF_ALLOCMEM(MFGpuImage, mfGetGpuImageSizeInBytes());
    
    MFGpuImageConfig config = {
        .width = width,
        .height = height,
        .pixels = pixels,
        .binding = MF_INFINITY
    };
    mfGpuImageCreate(tex, renderer, config);

    if(pixels != (blackColor))
        stbi_image_free(pixels);

    return tex;
}

MFArray mfMaterialSystemGetModelMatImages(MFModel* model, const char* basePath, void* renderer) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_ASSERT(basePath == mfnull, mfGetLogger(), "The base path provided shouldn't be null!");

    MFArray arr = mfArrayCreate(mfGetLogger(), MF_MODEL_MAT_TEXTURE_MAX, sizeof(MFGpuImage*));
    arr.len = MF_MODEL_MAT_TEXTURE_MAX;

    MFModelMaterial mat = model->mat;
    if(mat.alpha_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_ALPHA) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.alpha_texpath), renderer);    
    if(mat.ambient_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_AMBIENT) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.ambient_texpath), renderer);    
    if(mat.bump_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_BUMP) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.bump_texpath), renderer);    
    if(mat.diffuse_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_DIFFUSE) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.diffuse_texpath), renderer);    
    if(mat.displacement_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_DISPLACEMENT) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.displacement_texpath), renderer);
    if(mat.specular_highlight_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_SPECULAR_HIGHLIGHT) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.specular_highlight_texpath), renderer);
    if(mat.specular_texpath)
        mfArrayGet(arr, MFGpuImage*, MF_MODEL_MAT_TEXTURE_SPECULAR) = loadImage(mfStringConcatenate(mfGetLogger(), basePath, mat.specular_texpath), renderer);

    return arr;
}

void mfMaterialSystemDeleteModelMatImages(MFArray* array) {
    MF_ASSERT(array == mfnull, mfGetLogger(), "The provided MFArray handle shouldn't be null!");

    for(int i = 0; i < array->len; i++) {
        if(mfArrayGet(*array, MFGpuImage*, i) != mfnull)
            mfGpuImageDestroy(mfArrayGet(*array, MFGpuImage*, i));
    }

    mfArrayDestroy(array, mfGetLogger());
}