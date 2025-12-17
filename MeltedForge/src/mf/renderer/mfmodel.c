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

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder) {
    MF_ASSERT(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");

    // Assimp test
    {
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
                struct aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
                // Process these info ...
            }

            mfMeshCreate(&model->meshes[i], renderer, perVertSize * mesh->mNumVertices, vertices, mesh->mNumFaces * 3, indices);
            
            MF_FREEMEM(vertices);
            MF_FREEMEM(indices);
        }
        aiReleaseImport(scene);
    }

    // REMOVE tinyobjloader
    {
        tinyobj_attrib_t attrib;
        tinyobj_shape_t* shapes = mfnull;
        u64 shapesCount = 0;
        tinyobj_material_t* mats = mfnull;
        u64 matCount = 0;

        tinyobj_attrib_init(&attrib);

        int ret = tinyobj_parse_obj(&attrib, &shapes, &shapesCount, &mats, &matCount, filePath, &file_reader, mfnull, TINYOBJ_FLAG_TRIANGULATE);
        MF_ASSERT(ret != TINYOBJ_SUCCESS, mfGetLogger(), "Failed to load the model!");

        for (u64 s = 0; s < shapesCount; s++) {
            //! FIXME: FIX THIS TO NOT ONLY CONSIDER THE FIRST MATERIAL
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
        }

        tinyobj_attrib_free(&attrib);
    }
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