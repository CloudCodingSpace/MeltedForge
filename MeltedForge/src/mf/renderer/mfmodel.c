#include "mfmodel.h"

#include <stdint.h>

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

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder) { // TODO: Add a proper material system
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");

    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = mfnull;
    u64 shapesCount = 0;
    tinyobj_material_t* mats = mfnull;
    u64 matCount = 0;

    tinyobj_attrib_init(&attrib);

    int ret = tinyobj_parse_obj(
        &attrib,
        &shapes,
        &shapesCount,
        &mats,
        &matCount,
        filePath,
        &file_reader,
        mfnull,
        TINYOBJ_FLAG_TRIANGULATE
    );

    MF_ASSERT(ret != TINYOBJ_SUCCESS, mfGetLogger(), "Failed to load the model!");

    model->meshCount = shapesCount;
    model->meshes = MF_ALLOCMEM(MFMesh, sizeof(MFMesh) * shapesCount);

    for (size_t s = 0; s < shapesCount; ++s) {
        tinyobj_shape_t* shape = &shapes[s];
        u32 faceCount = shape->length;
        u32 vertexCount = faceCount * 3;

        u8* vertices = MF_ALLOCMEM(u8, vertexCount * perVertSize);
        u32* indices = MF_ALLOCMEM(u32, sizeof(u32) * vertexCount);

        for (u32 f = 0; f < faceCount; ++f) {
            for (u32 v = 0; v < 3; ++v) {
                u32 i = f * 3 + v;
                u32 globalIdx = shape->face_offset + i;

                tinyobj_vertex_index_t idx = attrib.faces[globalIdx];

                builder(vertices + i * perVertSize, &attrib, &idx);

                indices[i] = i; //! FIXME: Deduplicate the indices
            }
        }

        mfMeshCreate(&model->meshes[s], renderer, perVertSize * vertexCount, vertices, vertexCount, indices);

        MF_FREEMEM(vertices);
        MF_FREEMEM(indices);
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

void mfModelRender(MFModel* model) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");

    for(u64 i = 0; i < model->meshCount; i++) {
        mfMeshRender(&model->meshes[i]);
    }
}