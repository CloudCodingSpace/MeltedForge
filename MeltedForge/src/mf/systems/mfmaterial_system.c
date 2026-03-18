#include "mfmaterial_system.h"

#include <stb/stb_image.h>

MFGpuImage* loadImage(const char* path, void* renderer) {
    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    u8* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- %s", stbi_failure_reason());
        return mfCreateErrorGpuImage(renderer);
    }

    MFGpuImage* tex = MF_ALLOCMEM(MFGpuImage, mfGpuImageGetSizeInBytes());
    
    MFGpuImageConfig config = {
        .width = width,
        .height = height,
        .pixels = pixels,
        .binding = MF_INFINITY
    };
    mfGpuImageCreate(tex, renderer, config);

    return tex;
}

MFArray mfMaterialSystemLoadModelMatImages(MFModel* model, const char* basePath, MFRenderer* renderer) {
    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_PANIC_IF(model->meshCount <= 0, mfGetLogger(), "The model must have atleast 1 mesh!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(basePath == mfnull, mfGetLogger(), "The base path provided shouldn't be null!");
    
    b8 allocated = false;
    const char* bPath = basePath;
    char lastChar = basePath[mfStringLen(mfGetLogger(), basePath) - 1];
    if(lastChar != '\\' && lastChar != '/') {
        bPath = mfStringConcatenate(mfGetLogger(), basePath, "/");
        allocated = true;
    }

    MFArray list = mfArrayCreate(mfGetLogger(), model->meshCount, sizeof(MFArray));
    list.len = model->meshCount;

    for(u64 i = 0; i < model->meshCount; i++) {
        MFArray arr = mfArrayCreate(mfGetLogger(), MF_MODEL_MAT_TEXTURE_MAX, sizeof(MFGpuImage*));
        arr.len = MF_MODEL_MAT_TEXTURE_MAX;

        MFMeshMaterial mat = model->meshes[i].mat;

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

        for(int j = 0; j < MF_MODEL_MAT_TEXTURE_MAX; j++) {
            if(paths[j]) {
                const char* path = mfStringConcatenate(mfGetLogger(), bPath, paths[j]);

                mfArrayGet(arr, MFGpuImage*, j) = loadImage(path, renderer);
                MF_FREEMEM(path);
            }
        }
        
        mfArrayGet(list, MFArray, i) = arr;
    }
    
    if(allocated)
        MF_FREEMEM(bPath);

    return list;
}

void mfMaterialSystemDeleteModelMatImages(MFArray* array) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The provided MFArray handle shouldn't be null!");

    for(u64 i = 0; i < array->len; i++) {
        MFArray arr = mfArrayGet(*array, MFArray, i);
        for(u64 j = 0; j < arr.len; j++) {
            MFGpuImage* image = mfArrayGet(arr, MFGpuImage*, j);
            if(image != mfnull)
                mfGpuImageDestroy(image);
        }
    }

    mfArrayDestroy(array, mfGetLogger());
}

MFGpuImage* mfMaterialSystemGetImageFromArray(MFModelMatTextures type, MFArray* array, u64 meshIdx, MFRenderer* renderer) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be null!");
    MF_DO_IF((array->len == 0) || (array->elementSize != sizeof(MFArray)), {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The provided material texture array is invalid!");
        return mfnull;
    });
    MF_DO_IF((type < 0) || (type >= MF_MODEL_MAT_TEXTURE_MAX), {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The queried material texture's type is invalid!");
        return mfnull;
    });
    MF_DO_IF((meshIdx < 0) || (meshIdx >= array->len), {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The provided mesh index is an invalid index!");
        return mfnull;
    });
    
    MFGpuImage* image = mfArrayGet(mfArrayGet(*array, MFArray, meshIdx), MFGpuImage*, type);
    if(image == mfnull) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The queried material texture doesn't exists! Instead, using a error textured image!");
        mfArrayGet(mfArrayGet(*array, MFArray, meshIdx), MFGpuImage*, type) = mfCreateErrorGpuImage(renderer);
        image = mfArrayGet(*array, MFGpuImage*, type);
    }

    return image;
}