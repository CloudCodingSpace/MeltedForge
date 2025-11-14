#include "material_system.h"

#include "renderer/mfimage.h"
#include <stb/stb_image.h>

MFGpuImage* loadImage(const char* path, void* renderer) {
    u8 blackColor = 0x00;
    u32 width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    u8* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- \n");
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, stbi_failure_reason());
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "\n");
        pixels = &blackColor;
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

    if(pixels != (&blackColor))
        stbi_image_free(pixels);

    return tex;
}

MFArray mfMaterialSystemGetModelMatImages(MFModel* model, void* renderer) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_ASSERT(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");

    MFArray arr = mfArrayCreate(mfGetLogger(), 2, sizeof(MFGpuImage*));

    MFModelMaterial mat = model->mat;
    if(mat.alpha_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.alpha_texpath, renderer));
    if(mat.ambient_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.ambient_texpath, renderer));
    if(mat.bump_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.bump_texpath, renderer));
    if(mat.diffuse_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.diffuse_texpath, renderer));
    if(mat.displacement_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.displacement_texpath, renderer));
    if(mat.specular_highlight_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.specular_highlight_texpath, renderer));
    if(mat.specular_texpath)
        mfArrayAddElement(arr, MFGpuImage*, mfGetLogger(), loadImage(mat.specular_texpath, renderer));

    return arr;
}

void mfMaterialSystemDeleteModelMatImages(MFArray* array) {
    MF_ASSERT(array == mfnull, mfGetLogger(), "The provided MFArray handle shouldn't be null!");

    for(int i = 0; i < array->len; i++) {
        mfGpuImageDestroy(mfArrayGet(*array, MFGpuImage*, i));
    }

    mfArrayDestroy(array, mfGetLogger());
}