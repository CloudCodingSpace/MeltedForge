#include "mfmodel.h"

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

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
        MF_ASSERT(aiGetMaterialTexture(mat, type, 0, &path, mfnull, mfnull, mfnull, mfnull, mfnull, mfnull) != AI_SUCCESS,
                                            mfGetLogger(), "Couldn't retrieve the material's texture path from the model!");
        //! NOTE: SUS CUZ THE HEADER SAYS PATH.LENGTH IS THE BINARY LENGTH AND NOT THE LENGTH OF THE UTF-8 MULTI-BYTE SEQUENCE
        u64 size = path.length + 1;
        char* str = MF_ALLOCMEM(char, sizeof(char) * size);
        for(int i = 0; i < path.length; i++) {
            str[i] = path.data[i];
        }
        str[path.length] = '\0';

        return str;
    }

    return mfnull;
}

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");

    MF_SETMEM(&model->mat, 0, sizeof(MFModelMaterial));

    // Loading the model
    const struct aiScene* scene = aiImportFile(filePath, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeMeshes);
    MF_ASSERT((!scene) || (!scene->mRootNode), mfGetLogger(), aiGetErrorString());

    model->meshCount = scene->mNumMeshes;
    model->meshes = MF_ALLOCMEM(MFMesh, sizeof(MFMesh) * scene->mNumMeshes);

    //! NOTE: CHANGE THIS WAY OF DIRECTLY GETTING THE MESHES FROM THE SCENE, INSTEAD GET THE MESHES BASED ON THE HIERARCHY OF THE SCENE ALONG WITH THE TRANSFORMS!!
    for(int i = 0; i < scene->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[i];

        u8* vertices = MF_ALLOCMEM(u8, perVertSize * mesh->mNumVertices);
        u32* indices = MF_ALLOCMEM(u32, sizeof(u32) * mesh->mNumFaces * 3);

        for(int j = 0; j < mesh->mNumVertices; j++) {
            struct aiVector3D pos = mesh->mVertices[j];
            struct aiVector3D normals = mesh->mNormals[j];
            struct aiVector3D tangents = mesh->mTangents[j];

            struct aiVector3D texCoords = {0};
            if (mesh->mTextureCoords[0]) {
                texCoords = mesh->mTextureCoords[0][j];
            }

            // Process these info ...
            MFModelVertexBuilderData data = {
                .pos = (MFVec3){pos.x, pos.y, pos.z},
                .normal = (MFVec3){normals.x, normals.y, normals.z},
                .tangent = (MFVec3){tangents.x, tangents.y, tangents.z},
                .texCoord = (MFVec2){texCoords.x, texCoords.y}
            };
            
            builder(vertices + j * perVertSize, data);
        }
        for(int j = 0; j < mesh->mNumFaces; j++) {
            struct aiFace face = mesh->mFaces[j];
            // Process these info ...
            for(u32  k = 0; k < face.mNumIndices; k++) {
                indices[j * 3 + k] = face.mIndices[k];
            }
        }
        
        // Material
        {
            //! FIXME: FIX THIS WAY CUZ IT SETS THE MAT TO THE MODEL ITSELF, NOT TO THE MESHES & IT ALSO USES THE MAT OF THE LAST MESH. FIX THIS ASAP
            if(scene->mMaterials[mesh->mMaterialIndex] && (scene->mNumMaterials > 0)) {
                struct aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
                int c = 1;

                model->mat.ambient_texpath = get_materialtex(mat, aiTextureType_AMBIENT);
                model->mat.diffuse_texpath = get_materialtex(mat, aiTextureType_DIFFUSE);
                model->mat.displacement_texpath = get_materialtex(mat, aiTextureType_DISPLACEMENT);
                model->mat.specular_texpath = get_materialtex(mat, aiTextureType_SPECULAR);
                model->mat.bump_texpath = get_materialtex(mat, aiTextureType_HEIGHT);
                model->mat.shininess_texpath = get_materialtex(mat, aiTextureType_SHININESS);
                model->mat.emission_texpath = get_materialtex(mat, aiTextureType_EMISSIVE);
                model->mat.metalness_texpath = get_materialtex(mat, aiTextureType_METALNESS);
                model->mat.lightmap_texpath = get_materialtex(mat, aiTextureType_LIGHTMAP);

                struct aiColor4D color;
                if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color) == AI_SUCCESS) {
                    model->mat.specular[0] = color.r;
                    model->mat.specular[1] = color.g;
                    model->mat.specular[2] = color.b;
                }
                if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color) == AI_SUCCESS) {
                    model->mat.emission[0] = color.r;
                    model->mat.emission[1] = color.g;
                    model->mat.emission[2] = color.b;
                }
                if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
                    model->mat.diffuse[0] = color.r;
                    model->mat.diffuse[1] = color.g;
                    model->mat.diffuse[2] = color.b;
                }
                if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
                    model->mat.ambient[0] = color.r;
                    model->mat.ambient[1] = color.g;
                    model->mat.ambient[2] = color.b;
                }

                float f = 0.0f;
                if(aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &f) == AI_SUCCESS) {
                    model->mat.shininess = f;
                }
                f = 1.0f;
                if(aiGetMaterialFloat(mat, AI_MATKEY_REFRACTI, &f) == AI_SUCCESS) {
                    model->mat.ior = f;
                }
                f = 1.0f;
                if(aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &f) == AI_SUCCESS) {
                    model->mat.opaque = (f >= 1.0f);
                }
            }
        }

        mfMeshCreate(&model->meshes[i], renderer, perVertSize * mesh->mNumVertices, vertices, mesh->mNumFaces * 3, indices);
        
        MF_FREEMEM(vertices);
        MF_FREEMEM(indices);
    }
    aiReleaseImport(scene);
}

void mfModelDestroy(MFModel* model) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    
    for(u64 i = 0; i < model->meshCount; i++) {
        mfMeshDestroy(&model->meshes[i]);        
    }

    MF_FREEMEM(model->mat.bump_texpath);
    MF_FREEMEM(model->mat.alpha_texpath);
    MF_FREEMEM(model->mat.ambient_texpath);
    MF_FREEMEM(model->mat.diffuse_texpath);
    MF_FREEMEM(model->mat.emission_texpath);
    MF_FREEMEM(model->mat.lightmap_texpath);
    MF_FREEMEM(model->mat.specular_texpath);
    MF_FREEMEM(model->mat.metalness_texpath);
    MF_FREEMEM(model->mat.shininess_texpath);
    MF_FREEMEM(model->mat.displacement_texpath);

    if(model->meshes)
        MF_FREEMEM(model->meshes);

    MF_SETMEM(model, 0, sizeof(MFModel));
}