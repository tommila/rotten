
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

void gltf_readMeshData(memory_arena* tempArena, const cgltf_node* node,
                       const char* path, mesh_data* meshData, mat4* meshTransform) {
  cgltf_mesh* cgltfMesh = node->mesh;
  cgltf_node_transform_local(node, **meshTransform);
  if (cgltfMesh != NULL) {
    for (cgltf_size primitiveIndex = 0;
         primitiveIndex < cgltfMesh->primitives_count; ++primitiveIndex) {
      /// CGLTF PRIMITIVE DATA PARSING ///

      cgltf_primitive* primitive = &cgltfMesh->primitives[primitiveIndex];

      f32* positions = 0;
      f32* normals = 0;
      f32* colors = 0;
      f32* texCoords = 0;

      u32 colorNum = 0;
      u32 texCoordNum = 0;
      u32 posNum = 0;
      u32 normalNum = 0;

      u32 vertexNum = 0;
      u32 vertexComponentNum = 0;

      for (cgltf_size attributeIndex = 0;
           attributeIndex < primitive->attributes_count; ++attributeIndex) {
        cgltf_attribute* attribute = &primitive->attributes[attributeIndex];
        cgltf_accessor* accessor = attribute->data;

        cgltf_size numComponents = cgltf_num_components(accessor->type);
        if (attribute->type == cgltf_attribute_type_position) {  // POSITION
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (numComponents == 3)) {
            {
              vertexNum = attribute->data->count;
              posNum = attribute->data->count * 3;
              vertexComponentNum += 3;
              positions = pushArray(tempArena, posNum, f32);
	      ASSERT(cgltf_accessor_unpack_floats(accessor, positions, posNum));

            }
          } else {
            LOG(LOG_LEVEL_WARN,
                "Vertices attribute data format not supported, use vec3 "
                "f32\n");
          }
        } else if (attribute->type == cgltf_attribute_type_normal) {  // NORMAL
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (numComponents == 3)) {
            {
              normalNum = attribute->data->count * 3;
              vertexComponentNum += 3;
              normals = pushArray(tempArena, normalNum, f32);
	      ASSERT(cgltf_accessor_unpack_floats(accessor, normals, normalNum));

            }
          } else {
            LOG(LOG_LEVEL_WARN,
                "Vertices attribute data format not supported, use vec3 "
                "f32\n");
          }
        } else if (attribute->type ==
                   cgltf_attribute_type_texcoord) {  // TEXCOORD
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (accessor->type == cgltf_type_vec2)) {
            texCoordNum = attribute->data->count * 2;
            vertexComponentNum += 2;
            texCoords = pushArray(tempArena, texCoordNum, f32);
	    ASSERT(cgltf_accessor_unpack_floats(accessor, texCoords, texCoordNum));

          } else {
            LOG(LOG_LEVEL_WARN,
                "Texcoords attribute data format not supported, use vec2 "
                "f32\n");
          }
        } else if (attribute->type == cgltf_attribute_type_color) {  // COLOR
          if ((accessor->component_type == cgltf_component_type_r_32f) &&
              (accessor->type == cgltf_type_vec4)) {
            colorNum = attribute->data->count * 4;
            vertexComponentNum += 4;
            colors = pushArray(tempArena, colorNum, f32);

	    ASSERT(cgltf_accessor_unpack_floats(accessor, colors, colorNum));
          } else {
            LOG(LOG_LEVEL_WARN,
                "Texcoords attribute data format not supported, use vec2 "
                "f32\n");
          }
        }
      }

      const usize posMemSize = sizeof(f32) * posNum;
      const usize normalMemSize = sizeof(f32) * normalNum;
      const usize texMemSize = sizeof(f32) * texCoordNum;
      const usize colorMemSize = sizeof(f32) * colorNum;

      const usize vertexDataSize =
          posMemSize + normalMemSize + texMemSize + colorMemSize;

      meshData->vertexData = (f32*)pushSize(tempArena, vertexDataSize);
      f32 *vIt = meshData->vertexData;
      for (u32 vIdx = 0; vIdx < posNum / 3; vIdx++) {
	*vIt = positions[vIdx * 3];
	vIt++;
	*vIt = positions[vIdx * 3 + 1];
	vIt++;
	*vIt = positions[vIdx * 3 + 2];
	vIt++;

	if (normalNum) {
	  *vIt = normals[vIdx * 3];
	  vIt++;
	  *vIt = normals[vIdx * 3 + 1];
	  vIt++;
	  *vIt = normals[vIdx * 3 + 2];
	  vIt++;
	}

	if (texCoordNum) {
	  *vIt = texCoords[vIdx * 2];
	  vIt++;
	  *vIt = texCoords[vIdx * 2 + 1];
	  vIt++;
	}

	if (colorNum) {
	  *vIt = colors[vIdx * 4 ];
	  vIt++;
	  *vIt = colors[vIdx * 4 + 1];
	  vIt++;
	  *vIt = colors[vIdx * 4 + 2];
	  vIt++;
	  *vIt = colors[vIdx * 4 + 3];
	  vIt++;
	}
      }

      // u32 pOffset = 0;

      // memcpy(meshData->vertexData + pOffset, positions, posMemSize);
      // pOffset += posMemSize;
      // memcpy(meshData->vertexData + pOffset, normals, normalMemSize);
      // pOffset += normalMemSize;
      // memcpy(meshData->vertexData + pOffset, texCoords, texMemSize);
      // pOffset += texMemSize;
      // memcpy(meshData->vertexData + pOffset, colors, colorMemSize);

      meshData->vertexNum = vertexNum;
      meshData->vertexComponentNum = vertexComponentNum;

      meshData->indexNum = (primitive->indices != NULL)
                               ? primitive->indices->count
                               : vertexNum;

      meshData->indices = pushArray(tempArena, meshData->indexNum, u32);

      meshData->vertexDataSize = vertexDataSize;
      meshData->indexDataSize =  meshData->indexNum * sizeof(u32);

      if (primitive->indices != NULL) {
        cgltf_accessor* accessor = primitive->indices;
        LOG(LOG_LEVEL_VERBOSE, "Index amount %lu\n", accessor->count);
	ASSERT(cgltf_accessor_unpack_indices(accessor, meshData->indices, sizeof(u32), meshData->indexNum));
      } else {
        usize numIndices = 0;
        for (cgltf_size v = 0; v < vertexNum; v++) {
	  int16_t vertexIndex = int16_t(v);
	  ASSERT(v < meshData->indexNum);
	  meshData->indices[numIndices++] = vertexIndex;
        }
      }

      for (u16 iIdx = 0; iIdx < meshData->indexNum; iIdx++) {
        LOG(LOG_LEVEL_VERBOSE, "Indices %d: %d\n", iIdx,
            meshData->indices[iIdx]);
      }
    }
  }
}

void gltf_readMatData(const cgltf_node* node, const char* path,
                       mesh_material_data* matData) {
  cgltf_mesh* cgltfMesh = node->mesh;

  if (NULL != cgltfMesh) {
    for (cgltf_size primitiveIndex = 0;
         primitiveIndex < cgltfMesh->primitives_count; ++primitiveIndex) {
      cgltf_primitive* primitive = &cgltfMesh->primitives[primitiveIndex];
      const cgltf_material* mat = primitive->material;
      cgltf_texture* tex =
          mat->pbr_metallic_roughness.base_color_texture.texture;
      matData->imagePath[0] = '\0';
      if (tex) {
        // load image data
        CONCAT2(matData->imagePath, path, tex->image->uri);
      }

      const cgltf_float* col = mat->pbr_metallic_roughness.base_color_factor;
      memcpy(matData->baseColor, col, sizeof(f32) * 4);
    }
  }
}

cgltf_data* gltf_loadFile(memory_arena* tempArena, const char* filePath) {
  u32 fileSize = 0;
  void* data;
  loadFile(tempArena, filePath, false, data, &fileSize);
  cgltf_options options = {};
  cgltf_data* cgltfData = nullptr;

  ASSERT(cgltf_parse(&options, data, fileSize, &cgltfData) ==
         cgltf_result_success);
  ASSERT(cgltf_load_buffers(&options, cgltfData, filePath) ==
         cgltf_result_success);

  return cgltfData;
}
