#ifdef __cplusplus
extern "C" {
#endif

#include "mfmodel.h"

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

#include "core/mfmaths.h"

#include <cgltf/cgltf.h>

// #include <assimp/cimport.h>
// #include <assimp/scene.h>
// #include <assimp/postprocess.h>

// MFMat4 ToMat4(C_STRUCT aiMatrix4x4 m) {
//     return (MFMat4){
//         .data = {
//             m.a1, m.b1, m.c1, m.d1,
//             m.a2, m.b2, m.c2, m.d2,
//             m.a3, m.b3, m.c3, m.d3,
//             m.a4, m.b4, m.c4, m.d4
//         }
//     };
// }

// const char* ToString(struct aiString string) {
//     u64 size = string.length + 1;
//     char* str = MF_ALLOCMEM(char, sizeof(char) * size);
//     for(u32 i = 0; i < string.length; i++) {
//         str[i] = string.data[i];
//     }
//     str[string.length] = '\0';

//     return str;
// }

// const char* get_materialtex(const struct aiScene* scene, struct aiMaterial* mat, enum aiTextureType type) {
//     //! FIXME: MAKE USE OF ALL THE TEXTURE TYPES AVAILABLE!
//     int count = aiGetMaterialTextureCount(mat, type);
//     if(count >= 1) {
//         struct aiString path;
//         MF_PANIC_IF(aiGetMaterialTexture(mat, type, 0, &path, mfnull, mfnull, mfnull, mfnull, mfnull, mfnull) != AI_SUCCESS,
//                                             mfGetLogger(), "Couldn't retrieve the material's texture path from the model!");
//         //! NOTE: SUS CUZ THE HEADER SAYS PATH.LENGTH IS THE BINARY LENGTH AND NOT THE LENGTH OF THE UTF-8 MULTI-BYTE SEQUENCE, ASSUMING EACH ELEMENT OF CHAR IS 1BYTE
//         if(path.data[0] == '*') {
//             u64 idx = strtoull(path.data + 1, mfnull, 10);
//             const char* texPath = ToString(scene->mTextures[idx]->mFilename);
//             u64 texLen = strlen(texPath);
//             bool hasFormat = false;

//             u64 size = strlen(texPath) + 1;
//             if(strchr(texPath, '.') != 0) {
//                 hasFormat = true;
//             } else {
//                 size += strlen(scene->mTextures[idx]->achFormatHint) + 1; // 1 for '.'
//             }

//             char* str = MF_ALLOCMEM(char, sizeof(char) * size);
//             for(u32 i = 0; i < texLen; i++) {
//                 str[i] = texPath[i];
//             }

//             if(!hasFormat) {
//                 u64 formatLen = strlen(scene->mTextures[idx]->achFormatHint);
//                 u64 j = texLen;
//                 str[j++] = '.';
//                 memcpy(&str[j], scene->mTextures[idx]->achFormatHint, sizeof(char) * formatLen);
//             }

//             str[size - 1] = '\0';

//             MF_FREEMEM(texPath);
//             return str;
//         } else {
//             return ToString(path);
//         }
//     }

//     return mfnull;
// }

void processMesh(MFModel* model, cgltf_scene* scene, cgltf_mesh* mesh, MFMat4 transform) {
    MFMeshMaterial matData = {0};
    MF_SETMEM(matData.ambient, -1, sizeof(f32) * 3);
    MF_SETMEM(matData.specular, -1, sizeof(f32) * 3);
    MF_SETMEM(matData.emission, -1, sizeof(f32) * 3);
    MF_SETMEM(matData.diffuse, -1, sizeof(f32) * 3);

    u64 verticesCount = 0, indicesCount = 0;

    for(u64 i = 0; i < mesh->primitives_count; i++) {
        cgltf_primitive* primitive = &mesh->primitives[i];
        if(primitive->indices) {
            indicesCount += primitive->indices->count;
        }

        for(u64 j = 0; j < primitive->attributes_count; j++) {
            cgltf_attribute* attrib = &primitive->attributes[j];
            
            if(attrib->type == cgltf_attribute_type_position)
            {
                verticesCount += attrib->data->count;
                break;
            }
        }
    }

    if(verticesCount == 0)
        return;

    u8* vertices = MF_ALLOCMEM(u8, model->perVertexSize * verticesCount);
    u32* indices = MF_ALLOCMEM(u32, sizeof(u32) * indicesCount);

    u32 vertexOffset = 0;
    u32 indexOffset = 0;
    for(u64 i = 0; i < mesh->primitives_count; i++) {
        cgltf_primitive* prim = &mesh->primitives[i];
        cgltf_accessor* posAccessor = mfnull;
        cgltf_accessor* normalAccessor = mfnull;
        cgltf_accessor* uvAccessor = mfnull;
        cgltf_accessor* colorAccessor = mfnull;
        cgltf_accessor* tangentAccessor = mfnull;

        cgltf_accessor* indexAccessor = prim->indices;

        for(u64 j = 0; j < prim->attributes_count; j++) {
            cgltf_attribute* attrib = &prim->attributes[j];

            switch(attrib->type) {
                case cgltf_attribute_type_position:
                    posAccessor = attrib->data;
                    break;
                case cgltf_attribute_type_normal:
                    normalAccessor = attrib->data;
                    break;
                case cgltf_attribute_type_texcoord:
                    if(attrib->index == 0)
                        uvAccessor = attrib->data;
                    break;
                case cgltf_attribute_type_tangent:
                    tangentAccessor = attrib->data;
                    break;
                case cgltf_attribute_type_color:
                    colorAccessor = attrib->data;
                    break;
                default:
                    break;
            };
        }

        if(!posAccessor)
            continue;
            
        if(indexAccessor) {
            for (u64 j = 0; j < indexAccessor->count; j++) {
                indices[indexOffset + j] = (u32)cgltf_accessor_read_index(indexAccessor, j) + vertexOffset;
            }
        }

        for(u64 j = 0; j < posAccessor->count; j++) {
            float pos[3] = {0};
            float tangent[4] = {0};
            float uv[2] = {0};
            float normal[3] = {0};
            float color[3] = {0};

            cgltf_accessor_read_float(posAccessor, j, pos, 3);

            if(normalAccessor)
                cgltf_accessor_read_float(normalAccessor, j, normal, 3);
            if(uvAccessor)
                cgltf_accessor_read_float(uvAccessor, j, uv, 2);
            if(tangentAccessor)
                cgltf_accessor_read_float(tangentAccessor, j, tangent, 4);
            if(colorAccessor)
                cgltf_accessor_read_float(colorAccessor, j, color, 3);

            MFVec3 bitangent = {0};

            if(tangentAccessor && normalAccessor) {
                MFVec3 n = { normal[0], normal[1], normal[2] };
                MFVec3 t = { tangent[0], tangent[1], tangent[2] };

                bitangent = mfVec3MulScalar(mfVec3Cross(n, t), tangent[3]);
            }
            MFModelVertexBuilderData data = {
                .pos = { pos[0], pos[1], pos[2] },
                .normal = { normal[0], normal[1], normal[2] },
                .texCoord = { uv[0], uv[1] },
                .tangent = { tangent[0], tangent[1], tangent[2] },
                .bitangent = bitangent
            };

            model->builder(vertices + (vertexOffset + j) * model->perVertexSize, data);
        }
        
        vertexOffset += posAccessor->count;
        if(indexAccessor)
            indexOffset += indexAccessor->count;
    }

    // for(u32 j = 0; j < mesh->mNumVertices; j++) {
    //     struct aiVector3D pos = mesh->mVertices[j];
    //     struct aiVector3D normals = mesh->mNormals[j];
    //     struct aiVector3D tangents = mesh->mTangents[j];
    //     struct aiVector3D bitangents = mesh->mBitangents[j];

    //     struct aiVector3D texCoords = {0};
    //     if (mesh->mTextureCoords[0]) {
    //         texCoords = mesh->mTextureCoords[0][j];
    //     }

    //     MFModelVertexBuilderData data = {
    //         .pos = (MFVec3){ pos.x, pos.y, pos.z },
    //         .normal = (MFVec3){ normals.x, normals.y, normals.z },
    //         .texCoord = (MFVec2){ texCoords.x, texCoords.y },
    //         .tangent = (MFVec3){ tangents.x, tangents.y, tangents.z },
    //         .bitangent = (MFVec3){ bitangents.x, bitangents.y, bitangents.z }
    //     };
        
    //     model->builder(vertices + j * model->perVertexSize, data);
    // }
    // for(u32 j = 0; j < mesh->mNumFaces; j++) {
    //     struct aiFace face = mesh->mFaces[j];
    //     for(u32  k = 0; k < face.mNumIndices; k++) {
    //         indices[j * 3 + k] = face.mIndices[k];
    //     }
    // }
    
    // if(scene->mMaterials[mesh->mMaterialIndex] && (scene->mNumMaterials > 0)) {
    //     struct aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    //     matData.ambient_texpath = get_materialtex(scene, mat, aiTextureType_AMBIENT);
    //     matData.diffuse_texpath = get_materialtex(scene, mat, aiTextureType_DIFFUSE);
    //     matData.displacement_texpath = get_materialtex(scene, mat, aiTextureType_DISPLACEMENT);
    //     matData.specular_texpath = get_materialtex(scene, mat, aiTextureType_SPECULAR);
    //     matData.normal_texpath = get_materialtex(scene, mat, aiTextureType_NORMALS);
    //     matData.shininess_texpath = get_materialtex(scene, mat, aiTextureType_SHININESS);
    //     matData.emission_texpath = get_materialtex(scene, mat, aiTextureType_EMISSIVE);
    //     matData.metalness_texpath = get_materialtex(scene, mat, aiTextureType_METALNESS);
    //     matData.lightmap_texpath = get_materialtex(scene, mat, aiTextureType_LIGHTMAP);

    //     struct aiColor4D color;
    //     if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color) == AI_SUCCESS) {
    //         matData.specular[0] = color.r;
    //         matData.specular[1] = color.g;
    //         matData.specular[2] = color.b;
    //     }
    //     if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color) == AI_SUCCESS) {
    //         matData.emission[0] = color.r;
    //         matData.emission[1] = color.g;
    //         matData.emission[2] = color.b;
    //     }
    //     if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
    //         matData.diffuse[0] = color.r;
    //         matData.diffuse[1] = color.g;
    //         matData.diffuse[2] = color.b;
    //     }
    //     if(aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
    //         matData.ambient[0] = color.r;
    //         matData.ambient[1] = color.g;
    //         matData.ambient[2] = color.b;
    //     }

    //     float f = 0.0f;
    //     if(aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &f) == AI_SUCCESS) {
    //         matData.shininess = f;
    //     }
    //     f = 1.0f;
    //     if(aiGetMaterialFloat(mat, AI_MATKEY_REFRACTI, &f) == AI_SUCCESS) {
    //         matData.ior = f;
    //     }
    //     f = 1.0f;
    //     if(aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &f) == AI_SUCCESS) {
    //         matData.opaque = (f >= 1.0f);
    //     }
    // }

    model->meshes[model->_meshIdx].mat = matData;
    model->meshes[model->_meshIdx].transform = transform;
    mfMeshCreate(&model->meshes[model->_meshIdx], model->renderer, model->perVertexSize * verticesCount, vertices, indicesCount, indices);
    model->_meshIdx++;

    MF_FREEMEM(vertices);
    MF_FREEMEM(indices);
}

void processNode(MFModel* model, cgltf_scene* scene, cgltf_node* node, MFMat4 transform) {
    MFMat4 mat = transform;
    {
        MFMat4 out = mfMat4Identity();
        cgltf_node_transform_local(node, out.data);

        mat = mfMat4Mul(mat, out);
    }

    if(!node->mesh) {
        for(u32 i = 0; i < node->children_count; i++) {
            processNode(model, scene, node->children[i], mat);
        }
    }

    processMesh(model, scene, node->mesh, mat);
    // for(u32 i = 0; i < node->mesh; i++) {
    //     processMesh(model, scene, scene->mMeshes[node->mMeshes[i]], mat);
    // }
    // for(u32 i = 0; i < node->mNumChildren; i++) {
    //     processNode(model, scene, node->mChildren[i], mat);
    // }
}

void mfModelLoadAndCreate(MFModel* model, const char* filePath, MFRenderer* renderer, u64 perVertSize, MFModelVertexBuilder builder) {
    MF_PANIC_IF(model == mfnull, mfGetLogger(), "The model handle provided shouldn't be null!");
    MF_PANIC_IF(model->init, mfGetLogger(), "The model handle provided is already initialised!");

    MF_PANIC_IF(!mfStringEndsWith(mfGetLogger(), filePath, ".gltf"), mfGetLogger(), "The model must only be a .gltf file following glTF 2.0 standards. Glb aren't supported as of now!");

    // Loading the model
    cgltf_options options = {};
    cgltf_data* data;
    if(cgltf_parse_file(&options, filePath, &data) != cgltf_result_success) {
        MF_FATAL_ABORT(mfGetLogger(), "Failed to parse gltf file!");
    }
    
    if(cgltf_load_buffers(&options, data, filePath) != cgltf_result_success) {
        MF_FATAL_ABORT(mfGetLogger(), "Failed to parse gltf file!");
    }

    model->meshCount = data->meshes_count;
    model->meshes = MF_ALLOCMEM(MFMesh, sizeof(MFMesh) * data->meshes_count);
    model->renderer = renderer;
    model->builder = builder;
    model->perVertexSize = perVertSize;

    cgltf_scene* scene = data->scene;
    for(u32 i = 0; i < scene->nodes_count; i++) {
        processNode(model, scene, scene->nodes[i], mfMat4Identity());
    }

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

#ifdef __cplusplus
}
#endif