#ifdef __cplusplus
extern "C" {
#endif

#include "mfmaterial_system.h"

#include <stb/stb_image.h>
#include "core/mfhashmap.h"

typedef struct {
    u64 path_hash;
    u32 rgba;
    u8 type;
} TextureDescription;

typedef struct MFMaterialSystemState_s {
    bool init;
    MFHashMap map;
} MFMaterialSystemState;

static MFMaterialSystemState s_State = {0};

static MFGpuImage* loadImage(const char* path, MFModelMatTextures type, MFMeshMaterial* mat, void* renderer);
static u32 arrayToU32(f32* data);

void mfMaterialSystemInitialize(void) {
    if(s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system is already initialised!");
    }

    s_State.map = mfHashMapCreate(5, sizeof(TextureDescription), sizeof(MFGpuImage*));

    s_State.init = true;
}

void mfMaterialSystemShutdown(void) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }

    for(u64 i = 0; i < s_State.map.buckets.len; i++) {
        MFArray* bucket = &mfArrayGetElement(s_State.map.buckets, MFArray, i);
        for(u64 j = 0; j < bucket->len; j++) {
            MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, j);
            MFGpuImage** image = (MFGpuImage**)entry->value;
            mfGpuImageDestroy(*image);
        }
    }

    mfHashMapDestroy(&s_State.map);

    s_State.init = false;
    MF_SETMEM(&s_State, 0, sizeof(MFMaterialSystemState));
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
    
    bool allocated = false;
    const char* bPath = basePath;
    char lastChar = basePath[mfStringLen(basePath) - 1];
    if(lastChar != '\\' && lastChar != '/') {
        bPath = mfStringConcatenate(mfGetLogger(), basePath, "/");
        allocated = true;
    }

    MFArray list = mfArrayCreate(model->meshCount, sizeof(MFArray));
    list.len = model->meshCount;

    for(u64 i = 0; i < model->meshCount; i++) {
        MFArray arr = mfArrayCreate(MF_MODEL_MAT_TEXTURE_MAX, sizeof(MFGpuImage*));
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
            if(paths[j] && paths[j][0] != '\0') {
                char* path = mfStringConcatenate(mfGetLogger(), bPath, paths[j]);
                mfNormalizePath(path, mfGetLogger());

                f32 color[3] = {0x0};
                switch(j) {
                    case MF_MODEL_MAT_TEXTURE_DIFFUSE:
                        if(mat.diffuse[0] != -1)
                            memcpy(color, mat.diffuse, sizeof(f32) * 3);
                        break;
                    case MF_MODEL_MAT_TEXTURE_AMBIENT:
                        if(mat.ambient[0] != -1)
                            memcpy(color, mat.ambient, sizeof(f32) * 3);
                        break;
                    case MF_MODEL_MAT_TEXTURE_SPECULAR:
                        if(mat.specular[0] != -1)
                            memcpy(color, mat.specular, sizeof(f32) * 3);
                        break;
                    case MF_MODEL_MAT_TEXTURE_EMISSIVE:
                        if(mat.emission[0] != -1)
                            memcpy(color, mat.emission, sizeof(f32) * 3);
                        break;
                };

                TextureDescription description = {
                    .rgba = arrayToU32(color),
                    .path_hash = mfHash_FNV1A(path, sizeof(char) * mfStringLen(path)),
                    .type = (u8)j
                };
                MFGpuImage* image = (MFGpuImage*)mfHashMapGetValue(&s_State.map, &description);
                if(image == mfnull) {
                    image = loadImage(path, j, &mat, renderer);
                    mfHashMapAddElement(&s_State.map, &description, &image);
                }
                mfArraySetElement(arr, MFGpuImage*, j, image);

                MF_FREEMEM(path);
            }
        }
        
        mfArraySetElement(list, MFArray, i, arr);
    }
    
    if(allocated)
        MF_FREEMEM(bPath);

    return list;
}

void mfMaterialSystemDestroyModelMatImages(MFArray* array) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The provided MFArray handle shouldn't be null!");

    for(u64 i = 0; i < array->len; i++) {
        MFArray arr = mfArrayGetElement(*array, MFArray, i);
        mfArrayDestroy(&arr);
    }

    mfArrayDestroy(array);
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

    MF_DO_IF(type >= MF_MODEL_MAT_TEXTURE_MAX, {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The queried material texture's type is invalid!");
        return mfnull;
    });

    MF_DO_IF(meshIdx >= model->meshCount, {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "The provided mesh index is an invalid index!");
        return mfnull;
    });

    MFArray meshArray = mfArrayGetElement(*array, MFArray, meshIdx);
    MFGpuImage* image = mfArrayGetElement(meshArray, MFGpuImage*, type);

    if(image)
        return image;

    MFMeshMaterial* mat = &model->meshes[meshIdx].mat;

    f32 colorF[3] = {0};
    bool validColor = true;

    switch(type) {
        case MF_MODEL_MAT_TEXTURE_DIFFUSE:
            validColor = mat->diffuse[0] != -1;
            if(validColor) 
                memcpy(colorF, mat->diffuse, sizeof(colorF));
            break;

        case MF_MODEL_MAT_TEXTURE_AMBIENT:
            validColor = mat->ambient[0] != -1;
            if(validColor) 
                memcpy(colorF, mat->ambient, sizeof(colorF));
            break;

        case MF_MODEL_MAT_TEXTURE_SPECULAR:
            validColor = mat->specular[0] != -1;
            if(validColor) 
                memcpy(colorF, mat->specular, sizeof(colorF));
            break;

        case MF_MODEL_MAT_TEXTURE_EMISSIVE:
            validColor = mat->emission[0] != -1;
            if(validColor) 
                memcpy(colorF, mat->emission, sizeof(colorF));
            break;

        default:
            validColor = false;
            break;
    }

    u32 rgba = arrayToU32(colorF);

    TextureDescription desc = {
        .rgba = rgba,
        .path_hash = 0,
        .type = (u8)type
    };

    MFGpuImage* entry = (MFGpuImage*)mfHashMapGetValue(&s_State.map, &desc);
    if(entry != mfnull) {
        mfArraySetElement(meshArray, MFGpuImage*, type, entry);
        return entry;
    }

    MFGpuImage* img;

    if(!validColor) {
        for(u32 i = 0; i < 3; i++)
            colorF[i] = 1.0f;
    }
    u8 color[4] = {
        (u8)(colorF[0] * 255),
        (u8)(colorF[1] * 255),
        (u8)(colorF[2] * 255),
        255
    };

    MFGpuImageConfig config = {
        .binding = MF_INFINITY,
        .width = 1,
        .height = 1,
        .pixels = color,
        .imageFormat = MF_FORMAT_R8G8B8A8_UNORM
    };
    img = mfGpuImageCreate(renderer, config);

    mfHashMapAddElement(&s_State.map, &desc, &img);

    mfArraySetElement(meshArray, MFGpuImage*, type, img);
    return img;
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
            };
error_return:
        if(type != MF_MODEL_MAT_TEXTURE_NORMAL)
            return mfCreateErrorGpuImage(renderer);
        else {
            for(u32 i = 0; i < 4; i++)
                buff[i] = 0xff;
        }
        pixels = buff;
        width = 1;
        height = 1;
    } else {
        pixels = img_pixels;
    }
    
    MFGpuImageConfig config = {
        .width = width,
        .height = height,
        .pixels = pixels,
        .binding = MF_INFINITY,
        .imageFormat = MF_FORMAT_R8G8B8A8_UNORM,
        .generateMipmaps = true
    };

    if(type == MF_MODEL_MAT_TEXTURE_DIFFUSE) {
        config.generateMipmaps = true;
    }

    MFGpuImage* image = mfGpuImageCreate(renderer, config);
    if(pixels == img_pixels)
        stbi_image_free(pixels);

    return image;
}

static u32 arrayToU32(f32* data) {
    u8 r = (u8)round(data[0] * 255.0f);
    u8 g = (u8)round(data[1] * 255.0f);
    u8 b = (u8)round(data[2] * 255.0f);
    return (u32)((0xff << 24) | (g << 16) | (b << 8) | r);
}

#ifdef __cplusplus
}
#endif