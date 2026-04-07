#include "mfmaterial_system.h"

#include <stb/stb_image.h>
#include <ctype.h>

typedef struct {
    u64 path_hash;
    u32 rgba;
    char* path;
} TextureDescription;

typedef struct {
    u64 descHash;
    TextureDescription description;
    MFGpuImage* image;
} Entry;

typedef struct MFMaterialSystemState_s {
    b8 init;
    MFArray array;
} MFMaterialSystemState;

static MFMaterialSystemState s_State = {0};

static MFGpuImage* loadImage(const char* path, MFModelMatTextures type, MFMeshMaterial* mat, void* renderer);
static u64 FNV1A(const void* data, u64 size);
static b8 compareEntry(Entry* entry, const char* path, u32 rgba);
static Entry* findEntry(MFArray* array, const char* path, u32 rgba, u64 descHash);
static u32 arrayToU32(f32* data);
static void normalizePath(char* path);

void mfMaterialSystemInitialize(void) {
    if(s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system is already initialised!");
    }

    s_State.array = mfArrayCreate(mfGetLogger(), 2, sizeof(Entry));

    s_State.init = true;
}

void mfMaterialSystemShutdown(void) {
    if(!s_State.init) {
        MF_FATAL_ABORT(mfGetLogger(), "The material system isn't initialised!");
    }

    for(u64 i = 0; i < s_State.array.len; i++) {
        Entry* entry = &mfArrayGetElement(s_State.array, Entry, i);
        if(entry->description.path)
            MF_FREEMEM(entry->description.path);
        if(entry->image)
            mfGpuImageDestroy(entry->image);
        MF_SETMEM(entry, 0, sizeof(Entry));
    }

    mfArrayDestroy(&s_State.array, mfGetLogger());

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
            if(paths[j] && paths[j][0] != '\0') {
                char* path = mfStringConcatenate(mfGetLogger(), bPath, paths[j]);
                normalizePath(path);

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
                    .path = mfStringDuplicate(path),
                    .path_hash = FNV1A(path, sizeof(char) * mfStringLen(path))
                };
                u64 hashData[2] = { description.path_hash, description.rgba };
                u64 hash = FNV1A(hashData, sizeof(hashData));
                Entry* entry = findEntry(&s_State.array, path, description.rgba, hash);
                if(entry) {
                    mfArraySetElement(arr, MFGpuImage*, j, entry->image);
                    MF_FREEMEM(description.path);
                } else {
                    b8 inserted = false;

                    for(u64 k = 0; k < s_State.array.len; k++) {
                        Entry* e = &mfArrayGetElement(s_State.array, Entry, k);

                        if(hash < e->descHash) {
                            Entry newEntry = {
                                .description = description,
                                .descHash = hash,
                                .image = loadImage(path, j, &mat, renderer)
                            };

                            mfArrayInsertAt(&s_State.array, k, &newEntry, mfGetLogger());
                            mfArraySetElement(arr, MFGpuImage*, j, newEntry.image);
                            inserted = true;
                            break;
                        }
                    }

                    if(!inserted) {
                        Entry newEntry = {
                            .description = description,
                            .descHash = hash,
                            .image = loadImage(path, j, &mat, renderer)
                        };

                        mfArrayAddElement(s_State.array, Entry, mfGetLogger(), newEntry);
                        mfArraySetElement(arr, MFGpuImage*, j, newEntry.image);
                    }
                }

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
        mfArrayDestroy(&arr, mfGetLogger());
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
    b8 validColor = true;

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

    u32 rgba = validColor ? arrayToU32(colorF) : 0xff00ffff;

    TextureDescription desc = {
        .rgba = rgba,
        .path = mfnull,
        .path_hash = 0
    };

    u64 hashData[2] = { desc.path_hash, desc.rgba };
    u64 hash = FNV1A(hashData, sizeof(hashData));

    Entry* entry = findEntry(&s_State.array, mfnull, rgba, hash);

    if(entry) {
        mfArraySetElement(meshArray, MFGpuImage*, type, entry->image);
        return entry->image;
    }

    MFGpuImage* img;

    if(!validColor) {
        img = mfCreateErrorGpuImage(renderer);
    } else {
        u8 color[4] = {
            (u8)(colorF[0] * 255),
            (u8)(colorF[1] * 255),
            (u8)(colorF[2] * 255),
            255
        };

        img = MF_ALLOCMEM(MFGpuImage, mfGpuImageGetSizeInBytes());

        MFGpuImageConfig config = {
            .binding = MF_INFINITY,
            .width = 1,
            .height = 1,
            .pixels = color
        };
        mfGpuImageCreate(img, renderer, config);
    }

    Entry newEntry = {
        .descHash = hash,
        .description = desc,
        .image = img
    };

    b8 inserted = false;
    for(u64 k = 0; k < s_State.array.len; k++) {
        Entry* e = &mfArrayGetElement(s_State.array, Entry, k);

        if(hash < e->descHash) {
            mfArrayInsertAt(&s_State.array, k, &newEntry, mfGetLogger());
            inserted = true;
            break;
        }
    }

    if(!inserted) {
        mfArrayAddElement(s_State.array, Entry, mfGetLogger(), newEntry);
    }

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

// @note A little modified version of FNV-1a-64
static u64 FNV1A(const void* data, u64 size) {
    const u8 *p = (const u8*)data;

    u64 hash = 1469598103934665603ULL; // offset basis

    for (u64 i = 0; i < size; i++) {
        hash ^= p[i];
        hash *= 1099511628211ULL; // FNV prime
    }

    return hash;
}

static b8 compareEntry(Entry* entry, const char* path, u32 rgba) {
    u64 h = path ? FNV1A(path, sizeof(char) * mfStringLen(path)) : 0;
    if((entry->description.path_hash == h) &&
        (entry->description.rgba == rgba) && 
        (mfStringCompare(entry->description.path, path) == 0)) {
        return true;
    }

    return false;
}

static Entry* findEntry(MFArray* array, const char* path, u32 rgba, u64 descHash) {
    if(array->len == 0) 
        return mfnull;

    // u64 low = 0;
    // u64 high = array->len - 1;

    // while(low <= high) {
    //     u64 mid = (low + high) / 2;
    //     Entry* entry = &mfArrayGetElement(*array, Entry, mid);

    //     if(entry->descHash == descHash) {
    //         for(i64 i = (i64)mid; i >= 0; i--) {
    //             Entry* e = &mfArrayGetElement(*array, Entry, i);
    //             if(e->descHash != descHash) break;
    //             if(compareEntry(e, path, rgba)) return e;
    //         }

    //         for(u64 i = mid + 1; i < array->len; i++) {
    //             Entry* e = &mfArrayGetElement(*array, Entry, i);
    //             if(e->descHash != descHash) break;
    //             if(compareEntry(e, path, rgba)) return e;
    //         }

    //         return mfnull;
    //     }
    //     else if(entry->descHash < descHash)
    //         low = mid + 1;
    //     else
    //         high = mid - 1;
    // }

    for(u64 i = 0; i < array->len; i++) {
        Entry* entry = &mfArrayGetElement(*array, Entry, i);
        if(entry->descHash == descHash && compareEntry(entry, path, rgba))
            return entry;
    }

    return mfnull;
}

static u32 arrayToU32(f32* data) {
    u8 r = (u8)round(data[0] * 255.0f);
    u8 g = (u8)round(data[1] * 255.0f);
    u8 b = (u8)round(data[2] * 255.0f);
    return (u32)((0xff << 24) | (g << 16) | (b << 8) | r);
}

static void normalizePath(char* path) {
    char* src = path;
    char* dst = path;

    while (*src) {
        char c = *src;

        if (c == '\\')
            c = '/';

        c = (char)tolower((unsigned char)c);

        *dst++ = c;
        src++;
    }
    *dst = '\0';

    src = path;
    dst = path;

    while (*src) {
        *dst++ = *src;

        if (*src == '/') {
            while (*src == '/')
                src++;
        } else {
            src++;
        }
    }
    *dst = '\0';

    src = path;
    dst = path;

    while (*src) {
        if (src[0] == '.' && src[1] == '/') {
            src += 2;
            continue;
        }

        *dst++ = *src++;
    }
    *dst = '\0';

    size_t len = strlen(path);
    if (len > 1 && path[len - 1] == '/')
        path[len - 1] = '\0';
}