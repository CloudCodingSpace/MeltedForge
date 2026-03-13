#include "mfmaterial_system.h"

#include "renderer/mfgpuimage.h"
#include <stb/stb_image.h>

MFGpuImage* loadImage(const char* path, void* renderer) {
    u8 blackColor[4] = {0x00, 0x00, 0x00, 0xff};
    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    u8* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- %s", stbi_failure_reason());

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

    if(pixels != blackColor)
        stbi_image_free(pixels);

    return tex;
}

MFArray mfMaterialSystemGetModelMatImages(MFModel* model, const char* basePath, void* renderer) {
    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(basePath == mfnull, mfGetLogger(), "The base path provided shouldn't be null!");
    
    b8 allocated = false;
    const char* bPath = basePath;
    char lastChar = basePath[mfStringLen(mfGetLogger(), basePath) - 1];
    if(lastChar != '\\' && lastChar != '/') {
        bPath = mfStringConcatenate(mfGetLogger(), basePath, "/");
        allocated = true;
    }

    MFArray arr = mfArrayCreate(mfGetLogger(), MF_MODEL_MAT_TEXTURE_MAX, sizeof(MFGpuImage*));
    arr.len = MF_MODEL_MAT_TEXTURE_MAX;

    MFModelMaterial mat = model->mat;

    const char* paths[] = {
        mat.ambient_texpath,
        mat.diffuse_texpath,
        mat.specular_texpath,
        mat.bump_texpath,
        mat.displacement_texpath,
        mat.lightmap_texpath,
        mat.metalness_texpath,
        mat.shininess_texpath,
        mat.emission_texpath,
        mat.alpha_texpath
    };

    for(int i = 0; i < MF_MODEL_MAT_TEXTURE_MAX; i++) {
        if(paths[i]) {
            const char* path = mfStringConcatenate(mfGetLogger(), bPath, paths[i]);

            mfArrayGet(arr, MFGpuImage*, i) = loadImage(path, renderer);
            MF_FREEMEM(path);
        } else {
            mfArrayGet(arr, MFGpuImage*, i) = loadImage(mfnull, renderer); // mfnull, to load the default black image
        }
    }

    if(allocated)
        MF_FREEMEM(bPath);

    return arr;
}

void mfMaterialSystemDeleteModelMatImages(MFArray* array) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The provided MFArray handle shouldn't be null!");

    for(int i = 0; i < array->len; i++) {
        MFGpuImage* image = mfArrayGet(*array, MFGpuImage*, i);
        if(image != mfnull)
            mfGpuImageDestroy(image);
    }

    mfArrayDestroy(array, mfGetLogger());
}
