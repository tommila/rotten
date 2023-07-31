#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "cglm/types.h"
#include "cglm/vec3.h"
#include "cglm/vec4.h"

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "../ext/cgltf.h"
#include "../ext/stb_image.h"
#include "rotten.h"

#define LOAD_ATTRIBUTE(accesor, numComp, dataType, dstPtr)               \
  {                                                                      \
    int n = 0;                                                           \
    dataType* buffer = (dataType*)accesor->buffer_view->buffer->data +   \
                       accesor->buffer_view->offset / sizeof(dataType) + \
                       accesor->offset / sizeof(dataType);               \
    for (unsigned int k = 0; k < accesor->count; k++) {                  \
      for (int l = 0; l < numComp; l++) {                                \
        dstPtr[numComp * k + l] = buffer[n + l];                         \
      }                                                                  \
      n += (int)(accesor->stride / sizeof(dataType));                    \
    }                                                                    \
  }

void processGltfNode(res_model* model, platform_state* platform,
                     cgltf_node* node, const char* modelName) {
  cgltf_mesh* cgltfMesh = node->mesh;
  memory_block* meshMem = &platform->memory.meshMemBlock;

  if (NULL != cgltfMesh) {
    for (cgltf_size primitiveIndex = 0;
         primitiveIndex < cgltfMesh->primitives_count; ++primitiveIndex) {
      /// CGLTF PRIMITIVE DATA PARSING ///

      char meshId[128];
      CONCAT3(meshId, modelName, ":", cgltfMesh->name);

      res_mesh* res = findRes(meshMem, meshId, res_mesh);
      if (res) {
        SAFE_FREE(res->data.colors);
        SAFE_FREE(res->data.vertices);
        SAFE_FREE(res->data.texCoords);
      } else {
        res = pushSize(meshMem, res_mesh);
      }
      res->header.type = mesh;
      cgltf_node_transform_local(node, *res->transform);

      cgltf_primitive* primitive = &cgltfMesh->primitives[primitiveIndex];

      for (cgltf_size attributeIndex = 0;
           attributeIndex < primitive->attributes_count; ++attributeIndex) {
        cgltf_attribute* attribute = &primitive->attributes[attributeIndex];
        cgltf_accessor* accessor = attribute->data;

        cgltf_size numComponents = cgltf_num_components(accessor->type);

        if (attribute->type == cgltf_attribute_type_position &&
            attribute->index == 0) {  // POSITION
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (numComponents == 3)) {
            {
              res->data.vertexNum = attribute->data->count;
              res->data.vertices =
                  (float*)malloc(sizeof(float) * res->data.vertexNum * 3);
              //   int n = 0;
              //   int numComp = 3;
              //   float* buffer = (float*)accessor->buffer_view->buffer->data +
              //                   accessor->buffer_view->offset / sizeof(float)
              //                   + accessor->offset / sizeof(float);
              //   for (unsigned int k = 0; k < accessor->count; k++) {
              //     for (int l = 0; l < numComp; l++) {
              //       ASSERT(numComp * k + l < res->data.vertexNum * 3);
              //       res->data.vertices[numComp * k + l] = buffer[n + l];
              //     }
              //     n += (int)(accessor->stride / sizeof(float));
              //   }
              LOAD_ATTRIBUTE(accessor, 3, float, res->data.vertices);
            }
          } else {
            LOG(LOG_LEVEL_WARN,
                "Vertices attribute data format not supported, use vec3 "
                "float\n");
          }
        } else if (attribute->type ==
                   cgltf_attribute_type_texcoord) {  // TEXCOORD
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (accessor->type == cgltf_type_vec2)) {
            res->data.texCoordNum = attribute->data->count;
            res->data.texCoords =
                (float*)malloc(sizeof(float) * res->data.texCoordNum * 2);

            // int n = 0;
            // int numComp = 2;

            // float* buffer = (float*)accessor->buffer_view->buffer->data +
            //                 accessor->buffer_view->offset / sizeof(float) +
            //                 accessor->offset / sizeof(float);
            // for (unsigned int k = 0; k < accessor->count; k++) {
            //   for (int l = 0; l < numComp; l++) {
            //     ASSERT((numComp * k + l) < res->data.texCoordNum * 2);
            //     res->data.texCoords[numComp * k + l] = buffer[n + l];
            //   }
            //   n += (int)(accessor->stride / sizeof(float));
            // }

            LOAD_ATTRIBUTE(accessor, 2, float, res->data.texCoords);
          } else {
            LOG(LOG_LEVEL_WARN,
                "Texcoords attribute data format not supported, use vec2 "
                "float\n");
          }
        } else if (attribute->type == cgltf_attribute_type_color) {  // COLOR
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (accessor->type == cgltf_type_vec4)) {
            res->data.colorNum = attribute->data->count;
            res->data.colors =
                (float*)malloc(sizeof(float) * res->data.colorNum * 4);

            // int n = 0;
            // int numComp = 3;

            // float* buffer = (float*)accessor->buffer_view->buffer->data +
            //                 accessor->buffer_view->offset / sizeof(float) +
            //                 accessor->offset / sizeof(float);
            // for (unsigned int k = 0; k < accessor->count; k++) {
            //   for (int l = 0; l < numComp; l++) {
            //     ASSERT((numComp * k + l) < res->data.colorNum * 3);
            //     res->data.colors[numComp * k + l] = buffer[n + l];
            //   }
            //   n += (int)(accessor->stride / sizeof(float));
            // }

            LOAD_ATTRIBUTE(accessor, 4, float, res->data.colors);
          } else {
            LOG(LOG_LEVEL_WARN,
                "Texcoords attribute data format not supported, use vec2 "
                "float\n");
          }
        }
      }

      for (size_t i = 0; i < res->data.vertexNum * 3; i += 3) {
        LOG(LOG_LEVEL_VERBOSE, "vt %f %f %f\n", (double)res->data.vertices[i],
            (double)res->data.vertices[i + 1],
            (double)res->data.vertices[i + 2]);
      }

      for (size_t i = 0; i < res->data.colorNum * 4; i += 4) {
        LOG(LOG_LEVEL_VERBOSE, "vt %f %f %f\n", (double)res->data.colors[i],
            (double)res->data.colors[i + 1], (double)res->data.colors[i + 2],
            (double)res->data.colors[i + 3]);
      }

      for (size_t i = 0; i < res->data.texCoordNum * 2; i += 2) {
        LOG(LOG_LEVEL_VERBOSE, "tx %f %f\n", (double)res->data.texCoords[i],
            (double)res->data.texCoords[i + 1]);
      }

      /// BGFX MESH RES CREATION ///

      res->data.indexNum = (primitive->indices != NULL)
                               ? primitive->indices->count
                               : res->data.vertexNum;
      size_t indexMemSize = res->data.indexNum * sizeof(uint16_t);
      res->data.indices = (uint16_t*)malloc(indexMemSize);
      if (primitive->indices != NULL) {
        cgltf_accessor* accessor = primitive->indices;
        LOG(LOG_LEVEL_VERBOSE, "Index amount %lu\n", accessor->count);
        int n = 0;
        int numComp = 1;
        uint16_t* buffer = (uint16_t*)accessor->buffer_view->buffer->data +
                           accessor->buffer_view->offset / sizeof(uint16_t) +
                           accessor->offset / sizeof(uint16_t);
        for (unsigned int k = 0; k < accessor->count; k++) {
          for (int l = 0; l < numComp; l++) {
            ASSERT(numComp * k + l < res->data.indexNum);
            res->data.indices[numComp * k + l] = buffer[n + l];
          }
          n += (int)(accessor->stride / sizeof(uint16_t));
        }
        // LOAD_ATTRIBUTE(accessor, 1, uint16_t, res->data.indices);
      } else {
        size_t numIndices = 0;
        for (cgltf_size v = 0; v < res->data.vertexNum; v += 3) {
          for (int i = 0; i < 3; ++i) {
            int16_t vertexIndex = int16_t(v * 3 + i);
            ASSERT(v * 3 + i < res->data.indexNum);
            res->data.indices[numIndices++] = vertexIndex;
          }
        }
      }

      for (uint16_t iIdx = 0; iIdx < res->data.indexNum; iIdx++) {
        LOG(LOG_LEVEL_VERBOSE, "Indices %d: %d\n", iIdx,
            res->data.indices[iIdx]);
      }

      res_material* mat = findRes(&platform->memory.matMemBlock,
                                  primitive->material->name, res_material);
      stringCopy(meshId, res->header.name);
      LOG(LOG_LEVEL_INFO, "Mesh %s Imported\n", res->header.name);

      model->materials[model->meshNum] = mat;
      model->meshes[model->meshNum++] = res;
    }
  }
  for (cgltf_size childIndex = 0; childIndex < node->children_count;
       ++childIndex) {
    processGltfNode(model, platform, node->children[childIndex], modelName);
  }
}

bool importCgltfMaterials(platform_state* platform,
                          cgltf_material* cgltfMaterials, size_t matNum) {
  LOG(LOG_LEVEL_INFO, "Import Materials\n");
  memory_block* texMem = &platform->memory.texMemBlock;
  memory_block* matMem = &platform->memory.matMemBlock;
  for (size_t matIdx = 0; matIdx < matNum; matIdx++) {
    const cgltf_material* mat = &cgltfMaterials[matIdx];
    cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
    res_material* res = findRes(matMem, mat->name, res_material);
    if (!res) {
      res = pushSize(matMem, res_material);
    }
    res->header.type = material;
    res_texture* t = (tex) ? findRes(texMem, tex->image->uri, res_texture) : 0;

    res->texture = t;

    const cgltf_float* col = mat->pbr_metallic_roughness.base_color_factor;
    GLM_RGBA_SETP(col[0], col[1], col[2], col[3], &res->baseColor);

    stringCopy(mat->name, res->header.name);
    LOG(LOG_LEVEL_INFO, "Imported Material %s\n", mat->name);
  }
  return true;
}

bool importCgltfTextures(platform_state* platform, cgltf_texture* textures,
                         size_t textureNum, const char* path) {
  LOG(LOG_LEVEL_INFO, "Import Textures\n");
  for (size_t texIdx = 0; texIdx < textureNum; texIdx++) {
    const cgltf_image* image = textures[texIdx].image;
    const char* fileName = image->uri;
    char filePath[512];
    CONCAT2(filePath, path, fileName);
    importTexture(platform, fileName, filePath);
  }
  return true;
}
