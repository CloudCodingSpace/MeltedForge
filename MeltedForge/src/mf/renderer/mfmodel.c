#include "mfmodel.h"

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

typedef struct {
    tinyobj_vertex_index_t key;
    u32 value;
} VertexIndexMap;

void file_reader(void* ctx, const char* filename, int is_mtl, const char* base_dir, char** out_buf, uint64_t* out_len) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        *out_buf = NULL;
        *out_len = 0;
        return;
    }

    fseek(f, 0, SEEK_END);
    *out_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    *out_buf = malloc(*out_len + 1);
    fread(*out_buf, 1, *out_len, f);
    (*out_buf)[*out_len] = '\0';

    fclose(f);
}

static b8 vertex_index_cmp(const tinyobj_vertex_index_t* a, const tinyobj_vertex_index_t* b) {
    return a->v_idx == b->v_idx && a->vt_idx == b->vt_idx && a->vn_idx == b->vn_idx;
}

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");

    const struct aiScene* scene = aiImportFile(filePath, aiProcess_CalcTangentSpace | aiProcess_Triangulate);
    aiReleaseImport(scene);

    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = mfnull;
    u64 shapesCount = 0;
    tinyobj_material_t* mats = mfnull;
    u64 matCount = 0;

    tinyobj_attrib_init(&attrib);

    int ret = tinyobj_parse_obj(&attrib, &shapes, &shapesCount, &mats, &matCount, filePath, &file_reader, mfnull, TINYOBJ_FLAG_TRIANGULATE);
    MF_ASSERT(ret != TINYOBJ_SUCCESS, mfGetLogger(), "Failed to load the model!");

    model->meshCount = shapesCount;
    model->meshes = MF_ALLOCMEM(MFMesh, sizeof(MFMesh) * shapesCount);

    for (u64 s = 0; s < shapesCount; s++) {
        tinyobj_shape_t* shape = &shapes[s];
        u32 faceCount = shape->length;

        u8* vertices = MF_ALLOCMEM(u8, perVertSize * faceCount * 3);
        u32* indices = MF_ALLOCMEM(u32, sizeof(u32) * faceCount * 3);

        VertexIndexMap* remapTable = MF_ALLOCMEM(VertexIndexMap, sizeof(VertexIndexMap) * faceCount * 3);
        u32 remapCount = 0;
        u32 nextIndex = 0;

        for (u32 f = 0; f < faceCount; f++) {
            for (u32 v = 0; v < 3; v++) {
                u32 i = f * 3 + v;
                u32 globalIdx = shape->face_offset + i;
                tinyobj_vertex_index_t idx = attrib.faces[globalIdx];

                u32 found = 0;
                for (u32 k = 0; k < remapCount; k++) {
                    if (vertex_index_cmp(&remapTable[k].key, &idx)) {
                        indices[i] = remapTable[k].value;
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    builder(vertices + nextIndex * perVertSize, &attrib, &idx);

                    remapTable[remapCount].key = idx;
                    remapTable[remapCount].value = nextIndex;

                    indices[i] = nextIndex;

                    remapCount++;
                    nextIndex++;
                }
            }
        }

        // FIXME: FIX THIS TO NOT ONLY CONSIDER THE FIRST MATERIAL
        {
            if(matCount > 0) {
                tinyobj_material_t* mat = &mats[0];
            
                model->mat.alpha_texpath = mat->alpha_texname;
                model->mat.bump_texpath = mat->bump_texname;
                model->mat.ambient_texpath = mat->ambient_texname;
                model->mat.diffuse_texpath = mat->diffuse_texname;
                model->mat.specular_texpath = mat->specular_texname;
                model->mat.displacement_texpath = mat->displacement_texname;
                model->mat.specular_highlight_texpath = mat->specular_highlight_texname;

                model->mat.illum = mat->illum;
                model->mat.ior = mat->ior;
                model->mat.opaque = mat->dissolve;
                model->mat.shininess = mat->shininess;

                for(u32 i = 0; i < 3; i++) {
                    model->mat.ambient[i] = mat->ambient[i];
                    model->mat.diffuse[i] = mat->diffuse[i];
                    model->mat.emission[i] = mat->emission[i];
                    model->mat.specular[i] = mat->specular[i];
                    model->mat.transmittance[i] = mat->transmittance[i];
                }
            }
        }

        mfMeshCreate(&model->meshes[s], renderer, perVertSize * nextIndex, vertices, faceCount * 3, indices);

        MF_FREEMEM(vertices);
        MF_FREEMEM(indices);
        MF_FREEMEM(remapTable);
    }

    tinyobj_attrib_free(&attrib);
}

void mfModelDestroy(MFModel* model) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    
    for(u64 i = 0; i < model->meshCount; i++) {
        mfMeshDestroy(&model->meshes[i]);        
    }

    if(model->meshes)
        MF_FREEMEM(model->meshes);

    MF_SETMEM(model, 0, sizeof(MFModel));
}