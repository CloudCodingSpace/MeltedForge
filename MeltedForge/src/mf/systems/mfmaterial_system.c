#include "mfmaterial_system.h"

#include <stb/stb_image.h>

typedef struct MFMaterialSystemState_s {
    b8 init;
} MFMaterialSystemState;

static MFMaterialSystemState s_State = {0};

static MFGpuImage* loadImage(const char* path, MFModelMatTextures type, MFMeshMaterial* mat, void* renderer);

void mfMaterialSystemInitialize(void) {
    if(s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system is already initialised!");
    }

    s_State.init = true;
}

void mfMaterialSystemShutdown(void) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }

    s_State.init = false;
}

MFArray mfMaterialSystemLoadModelMatImages(MFModel* model, const char* basePath, MFRenderer* renderer) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }

    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_PANIC_IF(!model->init, mfGetLogger(), "The model handle provided isn't initialised!");
    MF_PANIC_IF(model->meshCount <= 0, mfGetLogger(), "The model must have atleast 1 mesh!");
    MF_PANIC_IF(renderer == mfnull, mfGetLogger(), "The renderer handle provided shouldn't be null!");
    MF_PANIC_IF(basePath == mfnull, mfGetLogger(), "The base path provided shouldn't be null!");
    
    b8 allocated = false;
    const char* bPath = basePath;
    char lastChar = basePath[mfStringLen(basePath) - 1];
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
            mat.normal_texpath,
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

                mfArraySetElement(arr, MFGpuImage*, j, loadImage(path, j, &mat, renderer));
                MF_FREEMEM(path);
            }
        }
        
        mfArraySetElement(list, MFArray, i, arr);
    }
    
    if(allocated)
        MF_FREEMEM(bPath);

    return list;
}

void mfMaterialSystemDeleteModelMatImages(MFArray* array) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The provided MFArray handle shouldn't be null!");

    for(u64 i = 0; i < array->len; i++) {
        MFArray arr = mfArrayGetElement(*array, MFArray, i);
        for(u64 j = 0; j < arr.len; j++) {
            MFGpuImage* image = mfArrayGetElement(arr, MFGpuImage*, j);
            if(image != mfnull)
                mfGpuImageDestroy(image);
        }
    }

    mfArrayDestroy(array, mfGetLogger());
}

MFGpuImage* mfMaterialSystemGetImageFromArray(MFModelMatTextures type, MFArray* array, MFModel* model, u64 meshIdx, MFRenderer* renderer) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }

    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be null!");
    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model provided shouldn't be null!");
    MF_PANIC_IF(!model->init, mfGetLogger(), "The model handle provided isn't initialised!");
    MF_DO_IF((array->len == 0) || (array->elementSize != sizeof(MFArray)), {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The provided material texture array is invalid!");
        return mfnull;
    });
    MF_DO_IF((type < 0) || (type >= MF_MODEL_MAT_TEXTURE_MAX), {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The queried material texture's type is invalid!");
        return mfnull;
    });
    MF_DO_IF((meshIdx < 0) || (meshIdx >= model->meshCount), {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The provided mesh index is an invalid index!");
        return mfnull;
    });
    
    MFGpuImage* image = mfArrayGetElement(mfArrayGetElement(*array, MFArray, meshIdx), MFGpuImage*, type);
    if(image == mfnull) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The queried material texture doesn't exists!");
        MFMeshMaterial* mat = &model->meshes[meshIdx].mat;
        u8 color[4] = {0x00, 0x00, 0x00, 0xff};
        
        switch(type) {
            case MF_MODEL_MAT_TEXTURE_DIFFUSE:
                if(mat->diffuse[0] == -1)
                    goto g_error;
                color[0] = (u8)mat->diffuse[0] * 255;
                color[1] = (u8)mat->diffuse[1] * 255;
                color[2] = (u8)mat->diffuse[2] * 255;
                break;
            case MF_MODEL_MAT_TEXTURE_AMBIENT:
                if(mat->ambient[0] == -1)
                    goto g_error;
                color[0] = (u8)mat->ambient[0] * 255;
                color[1] = (u8)mat->ambient[1] * 255;
                color[2] = (u8)mat->ambient[2] * 255;
                break;
            case MF_MODEL_MAT_TEXTURE_SPECULAR:
                if(mat->specular[0] == -1)
                    goto g_error;
                color[0] = (u8)mat->specular[0] * 255;
                color[1] = (u8)mat->specular[1] * 255;
                color[2] = (u8)mat->specular[2] * 255;
                break;
            case MF_MODEL_MAT_TEXTURE_EMISSIVE:
                if(mat->emission[0] == -1)
                    goto g_error;
                color[0] = (u8)mat->emission[0] * 255;
                color[1] = (u8)mat->emission[1] * 255;
                color[2] = (u8)mat->emission[2] * 255;
                break;
            default:
                goto g_error;
        };

        MFGpuImage* img = MF_ALLOCMEM(MFGpuImage, mfGpuImageGetSizeInBytes());
        mfGpuImageCreate(img, renderer, (MFGpuImageConfig){
            .binding = MF_INFINITY,
            .width = 1,
            .height = 1,
            .pixels = color
        });
        mfArraySetElement(mfArrayGetElement(*array, MFArray, meshIdx), MFGpuImage*, type, img);
        return img;

g_error:
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "Creating error texture!");
        mfArraySetElement(mfArrayGetElement(*array, MFArray, meshIdx), MFGpuImage*, type, mfCreateErrorGpuImage(renderer));
        image = mfArrayGetElement(mfArrayGetElement(*array, MFArray, meshIdx), MFGpuImage*, type);
    }

    return image;
}

static MFGpuImage* loadImage(const char* path, MFModelMatTextures type, MFMeshMaterial* mat, void* renderer) {
    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    u8 buff[4] = {0x00, 0x00, 0x00, 0xff};
    u8* pixels;
    u8* img_pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!img_pixels) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to load image! More reasons by image loader :- %s.", stbi_failure_reason());
        
        switch(type) {
            case MF_MODEL_MAT_TEXTURE_DIFFUSE:
                if(mat->diffuse[0] == -1)
                    goto error_return;
                buff[0] = (u8)mat->diffuse[0] * 255;
                buff[1] = (u8)mat->diffuse[1] * 255;
                buff[2] = (u8)mat->diffuse[2] * 255;
                break;
            case MF_MODEL_MAT_TEXTURE_AMBIENT:
                if(mat->ambient[0] == -1)
                    goto error_return;
                buff[0] = (u8)mat->ambient[0] * 255;
                buff[1] = (u8)mat->ambient[1] * 255;
                buff[2] = (u8)mat->ambient[2] * 255;
                break;
            case MF_MODEL_MAT_TEXTURE_SPECULAR:
                if(mat->specular[0] == -1)
                    goto error_return;
                buff[0] = (u8)mat->specular[0] * 255;
                buff[1] = (u8)mat->specular[1] * 255;
                buff[2] = (u8)mat->specular[2] * 255;
                break;
            case MF_MODEL_MAT_TEXTURE_EMISSIVE:
                if(mat->emission[0] == -1)
                    goto error_return;
                buff[0] = (u8)mat->emission[0] * 255;
                buff[1] = (u8)mat->emission[1] * 255;
                buff[2] = (u8)mat->emission[2] * 255;
                break;
error_return:
            if(type != MF_MODEL_MAT_TEXTURE_DISPLACEMENT && type != MF_MODEL_MAT_TEXTURE_NORMAL)
                return mfCreateErrorGpuImage(renderer);
            else {
                MF_FATAL_ABORT(mfGetLogger(), "Failed to load the normal/displacement material image!");
            }
        };
        pixels = buff;
        width = 1;
        height = 1;
    } else {
        pixels = img_pixels;
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