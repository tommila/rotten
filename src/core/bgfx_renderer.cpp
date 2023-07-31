#include <stdio.h>
#include <stdlib.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../platform.h"
#include "../rotten_platform.h"
#include "SDL2/SDL_log.h"
#define BGFX_P_H_HEADER_GUARD
#include "bgfx/bgfx.h"
#include "bgfx/c99/bgfx.h"
#include "cglm/affine.h"
#include "cglm/mat3.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"

void freeMeshBuffers(mesh_buffers* buffers) {
  bgfx_destroy_vertex_buffer({buffers->vertexBufferHandle});
  bgfx_destroy_vertex_layout({buffers->vertexLayoutHandle});
  bgfx_destroy_index_buffer({buffers->indexBufferHandle});
}

void freeShaderProgram(shader_program* program) {
  bgfx_destroy_shader({program->vertexShaderHandle});
  bgfx_destroy_shader({program->fragmentShaderHandle});
  bgfx_destroy_program({program->programHandle});
}

void freeMaterialBuffers(material_buffers* buffers) {
  bgfx_destroy_texture({buffers->textureHandle});
  bgfx_destroy_uniform({buffers->samplerHandle});
  bgfx_destroy_uniform({buffers->baseColorHandle});
}

bool rendererInit(uint16_t width, uint16_t height, void* window,
                  void* display) {
  // Init bgfx
  bgfx_init_t init;
  bgfx_init_ctor(&init);
  init.type = BGFX_RENDERER_TYPE_OPENGL;

  init.platformData.nwh = window;
  init.platformData.ndt = display;

  // bgfx::Init bgfx_init;
  init.resolution.height = height;
  init.resolution.width = width;
  init.resolution.reset = BGFX_RESET_VSYNC;

  bgfx_init(&init);
  bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f,
                      0);
  //  Enable debug text.
  // bgfx_set_debug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);
  // Set view 0 default viewport.
  bgfx_set_view_rect(0, 0, 0, width, height);
  // Set view 0 clear state.
  return true;
}

int counter = 0;

void rendererBegin(mat4* view, mat4* proj) {
  // Set view and projection matrix for view 0.
  bgfx_set_view_transform(0, *view, *proj);
  // Set view 0 default viewport.
  bgfx_set_view_rect(0, 0, 0, uint16_t(1920), uint16_t(1080));

  // This dummy draw call is here to make sure that view 0 is cleared
  // if no other draw calls are submitted to view 0.
  bgfx_touch(0);
}

void rendererFlip() { bgfx_frame(false); }

void renderMesh(mesh_buffers* mesh, material_buffers* material,
                shader_program* program, mat4 model) {
  bgfx_set_transform(*model, 1);
  bgfx_set_vertex_buffer(0, {mesh->vertexBufferHandle}, 0, UINT32_MAX);
  bgfx_set_index_buffer({mesh->indexBufferHandle}, 0, UINT32_MAX);
  if (material->textureHandle != UINT16_MAX) {
    bgfx_set_texture(0, {material->samplerHandle}, {material->textureHandle},
                     UINT32_MAX);
  }
  // bgfx_set_debug(BGFX_DEBUG_NONE);
  if (material->baseColorHandle != UINT16_MAX) {
    bgfx_set_uniform({material->baseColorHandle}, material->baseColor, 1);
  }
  bgfx_set_state(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                     BGFX_STATE_MSAA,
                 0);

  bgfx_submit(0, {program->programHandle}, 0, BGFX_DISCARD_ALL);
}

void renderMeshInstances(mesh_buffers* mesh, material_buffers* material,
                         shader_program* program, size_t modelNum, mat4* model,
                         vec4* color) {
  size_t instanceStride = sizeof(mat4) + sizeof(vec4);
  uint16_t drawnUnits =
      bgfx_get_avail_instance_data_buffer(modelNum, instanceStride);
  bgfx_instance_data_buffer_s idb;
  bgfx_alloc_instance_data_buffer(&idb, drawnUnits, instanceStride);

  uint8_t* data = idb.data;

  for (size_t i = 0; i < modelNum; ++i) {
    mat4* mtx = (mat4*)data;
    glm_mat4_copy(model[i], *mtx);
    data += instanceStride;
    vec4* c = (vec4*)&data[64];
    glm_vec4_copy(color[i], *c);
  }

  // Set vertex and index buffer.
  bgfx_set_vertex_buffer(0, {mesh->vertexBufferHandle}, 0, UINT32_MAX);
  bgfx_set_index_buffer({mesh->indexBufferHandle}, 0, UINT32_MAX);

  if (material->textureHandle != UINT16_MAX) {
    bgfx_set_texture(0, {material->samplerHandle}, {material->textureHandle},
                     UINT32_MAX);
  }
  // bgfx_set_debug(BGFX_DEBUG_NONE);
  if (material->baseColorHandle != UINT16_MAX) {
    bgfx_set_uniform({material->baseColorHandle}, material->baseColor, 1);
  }

  bgfx_set_state(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                     BGFX_STATE_MSAA,
                 0);

  // bgfx_set_debug(BGFX_DEBUG_WIREFRAME);
  bgfx_set_state(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                     BGFX_STATE_MSAA | BGFX_STATE_PT_LINESTRIP,
                 0);

  // Set instance data buffer.
  bgfx_set_instance_data_buffer(&idb, 0, UINT32_MAX);
  bgfx_submit(0, {program->programHandle}, 0, BGFX_DISCARD_ALL);
}

void rendererClose() { bgfx_shutdown(); }

bool initMeshBuffers(mesh_data* data, mesh_buffers* buffers) {
  const size_t vertexMemSize = sizeof(float) * data->vertexNum * 3 +
                               sizeof(float) * data->texCoordNum * 2;

  const bgfx_memory_t* vertexMem = bgfx_alloc((uint32_t)vertexMemSize);
  float* vertexData = (float*)vertexMem->data;

  const bgfx_memory_t* indexMem =
      bgfx_alloc((uint32_t)(sizeof(uint16_t) * data->indexNum));
  memcpy(indexMem->data, data->indices, indexMem->size);

  size_t vIt = 0;

  for (size_t vIdx = 0; vIdx < data->vertexNum; vIdx++) {
    vertexData[vIt++] = data->vertices[vIdx * 3];
    vertexData[vIt++] = data->vertices[vIdx * 3 + 1];
    vertexData[vIt++] = data->vertices[vIdx * 3 + 2];

    if (data->colorNum > 0) {
      vertexData[vIt++] = data->colors[vIdx * 4];
      vertexData[vIt++] = data->colors[vIdx * 4 + 1];
      vertexData[vIt++] = data->colors[vIdx * 4 + 2];
      vertexData[vIt++] = data->colors[vIdx * 4 + 3];
    }

    if (data->texCoordNum > 0) {
      vertexData[vIt++] = data->texCoords[vIdx * 2];
      vertexData[vIt++] = data->texCoords[vIdx * 2 + 1] - 1.0f;
    }
  }
  bgfx_vertex_layout_t layout;
  bgfx_vertex_layout_begin(&layout, BGFX_RENDERER_TYPE_NOOP);
  bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  if (data->colorNum > 0) {
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_COLOR0, 4,
                           BGFX_ATTRIB_TYPE_FLOAT, true, false);
  }
  if (data->texCoordNum > 0) {
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_TEXCOORD0, 2,
                           BGFX_ATTRIB_TYPE_FLOAT, true, false);
  }
  bgfx_vertex_layout_end(&layout);

  bgfx_vertex_layout_handle_t layoutHandle = bgfx_create_vertex_layout(&layout);
  bgfx_vertex_buffer_handle_t vbHandle =
      bgfx_create_vertex_buffer(vertexMem, &layout, BGFX_BUFFER_NONE);
  if (vbHandle.idx == UINT16_MAX) {
    SDL_LOG(LOG_LEVEL_ERROR, "BGFX Vertex Buffer creation failed\n");
    return false;
  }

  bgfx_index_buffer_handle_t idxHandle =
      bgfx_create_index_buffer(indexMem, BGFX_BUFFER_NONE);
  if (idxHandle.idx == UINT16_MAX) {
    SDL_LOG(LOG_LEVEL_ERROR, "BGFX Index Buffer creation failed\n");
    return false;
  }

  buffers->vertexLayoutHandle = layoutHandle.idx;
  buffers->vertexBufferHandle = vbHandle.idx;
  buffers->indexBufferHandle = idxHandle.idx;

  return true;
}

bool initShaderProgram(shader_data* data, shader_program* state) {
  bgfx_shader_handle_t vertexShaderHandle = bgfx_create_shader(
      bgfx_make_ref(data->vsData, (uint32_t)data->vsDataSize));
  bgfx_shader_handle_t fragmentShaderHandle = bgfx_create_shader(
      bgfx_make_ref(data->fsData, (uint32_t)data->fsDataSize));
  bgfx_program_handle_t programHandle;
  if (vertexShaderHandle.idx != UINT16_MAX &&
      fragmentShaderHandle.idx != UINT16_MAX) {
    programHandle =
        bgfx_create_program(vertexShaderHandle, fragmentShaderHandle, true);
    if (programHandle.idx == UINT16_MAX) {
      SDL_LOG(LOG_LEVEL_ERROR, "BGFX Shader creation failed\n");
      return false;
    }
  } else {
    SDL_LOG(LOG_LEVEL_ERROR, "BGFX Shader creation failed\n");
    return false;
  }
  state->vertexShaderHandle = vertexShaderHandle.idx;
  state->fragmentShaderHandle = fragmentShaderHandle.idx;
  state->programHandle = programHandle.idx;

  return true;
}

bool initMaterialBuffers(res_material* res, material_buffers* material) {
  material->baseColorHandle = UINT16_MAX;
  material->textureHandle = UINT16_MAX;
  material->samplerHandle = UINT16_MAX;

  if (res->texture) {
    img_data* imgData = &res->texture->data;
    const bgfx_memory_t* mem =
        bgfx_make_ref(imgData->pixels, (uint32_t)imgData->dataSize);

    bgfx_texture_format_t fmt;
    switch (imgData->components) {
      case 1:
        fmt = (imgData->depth == 8) ? BGFX_TEXTURE_FORMAT_R8
                                    : BGFX_TEXTURE_FORMAT_R16;
        break;
      case 2:
        fmt = (imgData->depth == 8) ? BGFX_TEXTURE_FORMAT_RG8
                                    : BGFX_TEXTURE_FORMAT_RG16;
        break;
      case 3:
        fmt = (imgData->depth == 8) ? BGFX_TEXTURE_FORMAT_RGB8
                                    : BGFX_TEXTURE_FORMAT_RGBA16;
        break;
      case 4:
        fmt = (imgData->depth == 8) ? BGFX_TEXTURE_FORMAT_RGBA8
                                    : BGFX_TEXTURE_FORMAT_RGBA16;
        break;
      default:
        break;
    }

    material->textureHandle =
        bgfx_create_texture_2d((uint16_t)imgData->width,
                               (uint16_t)imgData->height, false, 1, fmt,
                               BGFX_TEXTURE_NONE | BGFX_SAMPLER_POINT, NULL)
            .idx;
    material->samplerHandle =
        bgfx_create_uniform("s_texColor", BGFX_UNIFORM_TYPE_SAMPLER, 1).idx;

    material->baseColorHandle =
        bgfx_create_uniform("u_color", BGFX_UNIFORM_TYPE_VEC4, 1).idx;

    if (material->textureHandle == UINT16_MAX) {
      SDL_LOG(LOG_LEVEL_ERROR, "Texture creation failed\n");
      return false;
    }

    if (material->samplerHandle == UINT16_MAX) {
      SDL_LOG(LOG_LEVEL_ERROR, "Texture sample handle creation failed\n");
      return false;
    }

    if (material->baseColorHandle == UINT16_MAX) {
      SDL_LOG(LOG_LEVEL_ERROR, "Base color handle creation failed\n");
      return false;
    }

    bgfx_update_texture_2d({material->textureHandle}, 0, 0, 0, 0,
                           (uint16_t)imgData->width, (uint16_t)imgData->height,
                           mem, UINT16_MAX);

    if (material->textureHandle == UINT16_MAX) {
      SDL_LOG(LOG_LEVEL_ERROR, "Texture update failed\n");
      return false;
    }
  }
  glm_vec4_copy(res->baseColor, material->baseColor);

  return true;
}

bool updateMaterialBuffers(res_material* res, material_buffers* material) {
  if (res->texture) {
    img_data* imgData = &res->texture->data;
    const bgfx_memory_t* mem =
        bgfx_make_ref(imgData->pixels, (uint32_t)imgData->dataSize);

    bgfx_update_texture_2d({material->textureHandle}, 0, 0, 0, 0,
                           (uint16_t)imgData->width, (uint16_t)imgData->height,
                           mem, UINT16_MAX);

    if (material->textureHandle == UINT16_MAX) {
      SDL_LOG(LOG_LEVEL_ERROR, "Texture update failed\n");
      return false;
    }
  }
  glm_vec4_copy(res->baseColor, material->baseColor);

  return true;
}
