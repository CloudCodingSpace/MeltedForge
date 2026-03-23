#include "mfmodel.h"

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

#include "core/mfmaths.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

MFMat4 ToMat4(C_STRUCT aiMatrix4x4 m) {
    return (MFMat4){
        .data = {
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        }
    };
}

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

char* get_materialtex(struct aiMaterial* mat, enum aiTextureType type) {
    //! FIXME: MAKE USE OF ALL THE TEXTURE TYPES AVAILABLE!
    int count = aiGetMaterialTextureCount(mat, type);
    if(count >= 1) {
        struct aiString path;
        MF_PANIC_IF(aiGetMaterialTexture(mat, type, 0, &path, mfnull, mfnull, mfnull, mfnull, mfnull, mfnull) != AI_SUCCESS,
                                            mfGetLogger(), "Couldn't retrieve the material's texture path from the model!");
        //! NOTE: SUS CUZ THE HEADER SAYS PATH.LENGTH IS THE BINARY LENGTH AND NOT THE LENGTH OF THE UTF-8 MULTI-BYTE SEQUENCE
        u64 size = path.length + 1;
        char* str = MF_ALLOCMEM(char, sizeof(char) * size);
        for(u32 i = 0; i < path.length; i++) {
            str[i] = path.data[i];
        }
        str[path.length] = '\0';

        return str;
    }

    return mfnull;
}

void processMesh(MFModel* model, const struct aiScene* scene, struct aiMesh* mesh, MFMat4 transform) {
    MFMeshMaterial matData = {0};
    MF_SETMEM(matData.ambient, -1, sizeof(f32) * 3);
    MF_SETMEM(matData.specular, -1, sizeof(f32) * 3);
    MF_SETMEM(matData.emission, -1, sizeof(f32) * 3);
    MF_SETMEM(matData.diffuse, -1, sizeof(f32) * 3);

    u8* vertices = MF_ALLOCMEM(u8, model->perVertexSize * mesh->mNumVertices);
    u32* indices = MF_ALLOCMEM(u32, sizeof(u32) * mesh->mNumFaces * 3);

    for(u32 j = 0; j < mesh->mNumVertices; j++) {
        struct aiVector3D pos = mesh->mVertices[j];
        struct aiVector3D normals = mesh->mNormals[j];
        struct aiVector3D tangents = mesh->mTangents[j];

        struct aiVector3D texCoords = {0};
        if (mesh->mTextureCoords[0]) {
            texCoords = mesh->mTextureCoords[0][j];
        }

        MFModelVertexBuilderData data = {
            .pos = (MFVec3){pos.x, pos.y, pos.z},
            .normal = (MFVec3){normals.x, normals.y, normals.z},
            .tangent = (MFVec3){tangents.x, tangents.y, tangents.z},
            .texCoord = (MFVec2){texCoords.x, texCoords.y}
        };
        
        model->builder(vertices + j * model->perVertexSize, data);
    }
    for(u32 j = 0; j < mesh->mNumFaces; j++) {
        struct aiFace face = mesh->mFaces[j];
        for(u32  k = 0; k < face.mNumIndices; k++) {
            indices[j * 3 + k] = face.mIndices[k];
        }
    }
    
    if(scene->mMaterials[mesh->mMaterialIndex] && (scene->mNumMaterials > 0)) {
        struct aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        int c = 1;

        matData.ambient_texpath = get_materialtex(mat, aiTextureType_AMBIENT);
        matData.diffuse_texpath = get_materialtex(mat, aiTextureType_DIFFUSE);
        matData.displacement_texpath = get_materialtex(mat, aiTextureType_DISPLACEMENT);
        matData.specular_texpath = get_materialtex(mat, aiTextureType_SPECULAR);
        matData.bump_texpath = get_materialtex(mat, aiTextureType_HEIGHT);
        matData.shininess_texpath = get_materialtex(mat, aiTextureType_SHININESS);
        matData.emission_texpath = get_materialtex(mat, aiTextureType_EMISSIVE);
        matData.metalness_texpath = get_materialtex(mat, aiTextureType_METALNESS);
        matData.lightmap_texpath = get_materialtex(mat, aiTextureType_LIGHTMAP);

        struct aiColor4D color;
        if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color) == AI_SUCCESS) {
            matData.specular[0] = color.r;
            matData.specular[1] = color.g;
            matData.specular[2] = color.b;
        }
        if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color) == AI_SUCCESS) {
            matData.emission[0] = color.r;
            matData.emission[1] = color.g;
            matData.emission[2] = color.b;
        }
        if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
            matData.diffuse[0] = color.r;
            matData.diffuse[1] = color.g;
            matData.diffuse[2] = color.b;
        }
        if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
            matData.ambient[0] = color.r;
            matData.ambient[1] = color.g;
            matData.ambient[2] = color.b;
        }

        float f = 0.0f;
        if(aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &f) == AI_SUCCESS) {
            matData.shininess = f;
        }
        f = 1.0f;
        if(aiGetMaterialFloat(mat, AI_MATKEY_REFRACTI, &f) == AI_SUCCESS) {
            matData.ior = f;
        }
        f = 1.0f;
        if(aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &f) == AI_SUCCESS) {
            matData.opaque = (f >= 1.0f);
        }
    }

    model->meshes[model->meshIdx].mat = matData;
    model->meshes[model->meshIdx].transform = transform;
    mfMeshCreate(&model->meshes[model->meshIdx], model->renderer, model->perVertexSize * mesh->mNumVertices, vertices, mesh->mNumFaces * 3, indices);
    model->meshIdx++;

    MF_FREEMEM(vertices);
    MF_FREEMEM(indices);
}

void processNode(MFModel* model, const struct aiScene* scene, struct aiNode* node, MFMat4 transform) {
    MFMat4 mat = mfMat4Mul(transform, ToMat4(node->mTransformation));
    for(u32 i = 0; i < node->mNumMeshes; i++) {
        processMesh(model, scene, scene->mMeshes[node->mMeshes[i]], mat);
    }
    for(u32 i = 0; i < node->mNumChildren; i++) {
        processNode(model, scene, node->mChildren[i], mat);
    }
}

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder) {
    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_PANIC_IF(model->init, mfGetLogger(), "The model handle provided is already initialised!");

    // Loading the model
    const struct aiScene* scene = aiImportFile(filePath, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeMeshes);
    MF_PANIC_IF((!scene) || (!scene->mRootNode) || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE), mfGetLogger(), aiGetErrorString());

    model->meshCount = scene->mNumMeshes;
    model->meshes = MF_ALLOCMEM(MFMesh, sizeof(MFMesh) * scene->mNumMeshes);
    model->renderer = renderer;
    model->builder = builder;
    model->perVertexSize = perVertSize;

    processNode(model, scene, scene->mRootNode, mfMat4Identity());

    aiReleaseImport(scene);
    model->init = true;
}

void mfModelDestroy(MFModel* model) {
    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_PANIC_IF(!model->init, mfGetLogger(), "The model handle provided isn't initialised!");
    
    for(u64 i = 0; i < model->meshCount; i++) {
        mfMeshDestroy(&model->meshes[i]);
    }

    if(model->meshes)
        MF_FREEMEM(model->meshes);

    MF_SETMEM(model, 0, sizeof(MFModel));
}