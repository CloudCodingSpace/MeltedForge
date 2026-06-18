#ifdef __cplusplus
extern "C" {
#endif

#include "mfmodel.h"

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

#include "core/mfmaths.h"

#include <cgltf/cgltf.h>

const char* get_materialtex(cgltf_texture* tex) {
    if(!tex)
        return mfnull;
    
    cgltf_image* image = tex->image;
    if(!image)
        return mfnull;
    
    if(!image->uri)
        return mfnull;

    return mfStringDuplicate(image->uri);
}

void processMesh(MFModel* model, cgltf_scene* scene, cgltf_mesh* mesh, MFMat4 transform) {
    for(u64 i = 0; i < mesh->primitives_count; i++) {
        MFMeshMaterial matData = {0};
        MF_SETMEM(matData.emission, -1, sizeof(f32) * 3);
        MF_SETMEM(matData.diffuse, -1, sizeof(f32) * 3);

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
        
        if(!indexAccessor)
            continue;
        
        u8* vertices = MF_ALLOCMEM(u8, model->perVertexSize * posAccessor->count);
        u32* indices = MF_ALLOCMEM(u32, sizeof(u32) * indexAccessor->count);

        MFVec3* tanAccum = MF_ALLOCMEM(MFVec3, sizeof(MFVec3) * posAccessor->count);
        MFVec3* bitanAccum = MF_ALLOCMEM(MFVec3, sizeof(MFVec3) * posAccessor->count);
        MFVec4* generatedTangents = mfnull;

        for (u64 j = 0; j < indexAccessor->count; j++) {
            indices[j] = (u32)cgltf_accessor_read_index(indexAccessor, j);
        }

        if(!tangentAccessor && normalAccessor && uvAccessor) {
            generatedTangents = MF_ALLOCMEM(MFVec4, sizeof(MFVec4) * posAccessor->count);

            for(u64 j = 0; j < indexAccessor->count; j += 3) {
                u32 i0 = indices[j + 0];
                u32 i1 = indices[j + 1];
                u32 i2 = indices[j + 2];

                float p0f[3], p1f[3], p2f[3];
                float uv0f[2], uv1f[2], uv2f[2];

                cgltf_accessor_read_float(posAccessor, i0, p0f, 3);
                cgltf_accessor_read_float(posAccessor, i1, p1f, 3);
                cgltf_accessor_read_float(posAccessor, i2, p2f, 3);

                cgltf_accessor_read_float(uvAccessor, i0, uv0f, 2);
                cgltf_accessor_read_float(uvAccessor, i1, uv1f, 2);
                cgltf_accessor_read_float(uvAccessor, i2, uv2f, 2);

                MFVec3 p0 = { p0f[0], p0f[1], p0f[2] };
                MFVec3 p1 = { p1f[0], p1f[1], p1f[2] };
                MFVec3 p2 = { p2f[0], p2f[1], p2f[2] };

                MFVec2 uv0 = { uv0f[0], uv0f[1] };
                MFVec2 uv1 = { uv1f[0], uv1f[1] };
                MFVec2 uv2 = { uv2f[0], uv2f[1] };

                MFVec3 edge1 = mfVec3Sub(p1, p0);
                MFVec3 edge2 = mfVec3Sub(p2, p0);

                MFVec2 duv1 = mfVec2Sub(uv1, uv0);
                MFVec2 duv2 = mfVec2Sub(uv2, uv0);

                float det = duv1.x * duv2.y - duv2.x * duv1.y;

                if(fabsf(det) < 1e-6f)
                    continue;

                float f = 1.0f / det;

                MFVec3 tangent = {
                    f * (edge1.x * duv2.y - edge2.x * duv1.y),
                    f * (edge1.y * duv2.y - edge2.y * duv1.y),
                    f * (edge1.z * duv2.y - edge2.z * duv1.y)
                };

                MFVec3 bitangent = {
                    f * (edge2.x * duv1.x - edge1.x * duv2.x),
                    f * (edge2.y * duv1.x - edge1.y * duv2.x),
                    f * (edge2.z * duv1.x - edge1.z * duv2.x)
                };

                tanAccum[i0] = mfVec3Add(tanAccum[i0], tangent);
                tanAccum[i1] = mfVec3Add(tanAccum[i1], tangent);
                tanAccum[i2] = mfVec3Add(tanAccum[i2], tangent);

                bitanAccum[i0] = mfVec3Add(bitanAccum[i0], bitangent);
                bitanAccum[i1] = mfVec3Add(bitanAccum[i1], bitangent);
                bitanAccum[i2] = mfVec3Add(bitanAccum[i2], bitangent);
            }

            for(u64 j = 0; j < posAccessor->count; j++) {
                float normalf[3];

                cgltf_accessor_read_float(normalAccessor, j, normalf, 3);

                MFVec3 N = { normalf[0], normalf[1], normalf[2]};

                MFVec3 T = mfVec3Normalize(tanAccum[j]);

                MFVec3 B = mfVec3Normalize(bitanAccum[j]);

                float w = mfVec3Dot(mfVec3Cross(N, T), B) < 0.0f ? -1.0f : 1.0f;

                generatedTangents[j] = (MFVec4){ T.x, T.y, T.z, w };
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

            MFVec3 n = { normal[0], normal[1], normal[2] };
            if(tangentAccessor && normalAccessor) {
                MFVec3 t = { tangent[0], tangent[1], tangent[2] };

                bitangent = mfVec3MulScalar(mfVec3Cross(n, t), tangent[3]);
            }
            else if(generatedTangents) {
                tangent[0] = generatedTangents[j].x;
                tangent[1] = generatedTangents[j].y;
                tangent[2] = generatedTangents[j].z;
                tangent[3] = generatedTangents[j].w;

                MFVec3 t = { tangent[0], tangent[1], tangent[2] };
                bitangent = mfVec3MulScalar(mfVec3Cross(n, t), tangent[3]);
            }
            MFModelVertexBuilderData data = {
                .pos = { pos[0], pos[1], pos[2] },
                .normal = { normal[0], normal[1], normal[2] },
                .texCoord = { uv[0], 1.0f - uv[1] },
                .tangent = { tangent[0], tangent[1], tangent[2], tangent[3] },
                .bitangent = bitangent
            };

            model->builder(vertices + j * model->perVertexSize, data);
        }

        cgltf_material* mat = prim->material;
        if(mat) {
            memcpy(matData.diffuse, mat->pbr_metallic_roughness.base_color_factor, sizeof(f32) * 3);
            memcpy(matData.emission, mat->emissive_factor, sizeof(f32) * 3);
            
            matData.metallic = mat->pbr_metallic_roughness.metallic_factor;
            matData.roughness = mat->pbr_metallic_roughness.roughness_factor;

            if(mat->has_pbr_metallic_roughness) {
                matData.diffuse_texpath = get_materialtex(mat->pbr_metallic_roughness.base_color_texture.texture);
                matData.metallic_roughness_texpath = get_materialtex(mat->pbr_metallic_roughness.metallic_roughness_texture.texture);
            }
            matData.emission_texpath = get_materialtex(mat->emissive_texture.texture);
            matData.lightmap_texpath = get_materialtex(mat->occlusion_texture.texture);
            matData.normal_texpath = get_materialtex(mat->normal_texture.texture);
        }
    
        model->meshes[model->_meshIdx].mat = matData;
        model->meshes[model->_meshIdx].transform = transform;
        mfMeshCreate(&model->meshes[model->_meshIdx], model->renderer, model->perVertexSize * posAccessor->count, vertices, indexAccessor->count, indices);
        model->_meshIdx++;

        if(generatedTangents)
            MF_FREEMEM(generatedTangents);
        MF_FREEMEM(vertices);
        MF_FREEMEM(indices);
        MF_FREEMEM(tanAccum);
        MF_FREEMEM(bitanAccum);
    }
}

void processNode(MFModel* model, cgltf_scene* scene, cgltf_node* node) {
    if(!node)
        return;

    MFMat4 mat = mfMat4Identity();
    if(node->has_translation || node->has_rotation || node->has_scale) {
        cgltf_node_transform_world(node, mat.data);
    }

    if(node->mesh)
        processMesh(model, scene, node->mesh, mat);
    for(u32 i = 0; i < node->children_count; i++) {
        processNode(model, scene, node->children[i]);
    }
}

u64 getNodeMeshCount(cgltf_node* node) {
    cgltf_mesh* mesh = node->mesh;
    u64 count = 0;
    if(mesh) {
        count += mesh->primitives_count;
    }

    for(u64 i = 0; i < node->children_count; i++) {
        cgltf_node* child = node->children[i];
        count += getNodeMeshCount(child);
    }

    return count;
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

    cgltf_scene* scene = data->scene;

    for(u32 i = 0; i < scene->nodes_count; i++)
        model->meshCount += getNodeMeshCount(scene->nodes[i]);
    model->meshes = MF_ALLOCMEM(MFMesh, sizeof(MFMesh) * model->meshCount);
    model->renderer = renderer;
    model->builder = builder;
    model->perVertexSize = perVertSize;

    for(u32 i = 0; i < scene->nodes_count; i++) {
        processNode(model, scene, scene->nodes[i]);
    }

    cgltf_free(data);
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