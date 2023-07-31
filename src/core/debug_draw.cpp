
#include <cstdio>
#include <vector>

#include "../platform.h"
#include "bgfx/bgfx.h"
#include "bgfx/c99/bgfx.h"
#include "bgfx/defines.h"
#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include "cglm/vec3.h"
#include "cglm/vec4.h"

static vec3 sQuadVertices[4] = {
    {-0.5f, 0.5f, 0.0f},
    {0.5f, 0.5f, 0.0f},
    {0.5f, -0.5f, 0.0f},
    {-0.5f, -0.5f, 0.0f},
};

static const uint16_t sQuadIndices[9] = {0, 1, 2, 1, 3, 2, 3, 0};

// struct debugDraw_Rectangle {
//   bgfx_vertex_buffer_handle_t vbh;
//   bgfx_index_buffer_handle_t ibh;
//   bgfx_instance_data_buffer_t idb;
//   bgfx_vertex_layout_t layout;
//   bgfx_uniform_handle_t colorH;
//   bgfx_res_shader *shader;
// };

// static debugDraw_Rectangle rectangle;

void DEBUG_Init(void *meshMem) {
  bgfx_res_mesh *newMesh =
      (bgfx_res_mesh *)pushBGFXRes(meshMem, sizeof(bgfx_res_mesh));

  // bgfx_vertex_layout_begin(&rectangle.layout, BGFX_RENDERER_TYPE_NOOP);
  // bgfx_vertex_layout_add(&rectangle.layout, BGFX_ATTRIB_POSITION, 3,
  //                        BGFX_ATTRIB_TYPE_FLOAT, false, false);
  // // bgfx_vertex_layout_add(&rectangle.layout, BGFX_ATTRIB_TEXCOORD0, 2,
  // //                        BGFX_ATTRIB_TYPE_FLOAT, true, false);
  // bgfx_vertex_layout_end(&rectangle.layout);
  // rectangle.colorH = bgfx_create_uniform("u_color", BGFX_UNIFORM_TYPE_VEC4,
  // 1); rectangle.shader =
  //     (bgfx_res_shader *)findBGFXRes(shaderMem, "DebugDrawInstanced");
  // // // rectangle.vsCube =
  // // //     loadShader("debug_draw_instanced.vs.sc.bin",
  // "./assets/shaders/");
  // // // rectangle.fsCube =
  // // //     loadShader("debug_draw_instanced.fs.sc.bin",
  // "./assets/shaders/");
  // // // rectangle.program =
  // // //     bgfx::createProgram(rectangle.vsCube, rectangle.fsCube, true);

  // rectangle.vbh = bgfx_create_vertex_buffer(
  //     bgfx_make_ref(sQuadVertices, sizeof(sQuadVertices)), &rectangle.layout,
  //     BGFX_BUFFER_NONE);
  // rectangle.ibh = bgfx_create_index_buffer(
  //     bgfx_make_ref(sQuadIndices, sizeof(sQuadIndices)), BGFX_BUFFER_NONE);

  // rectangle.vbh = bgfx::createVertexBuffer(
  //     bgfx::makeRef(sQuadVertices, sizeof(sQuadVertices)), rectangle.layout);

  // rectangle.ibh = bgfx::createIndexBuffer(
  //     // Static data can be passed with bgfx::makeRef
  //     bgfx::makeRef(sQuadIndices, sizeof(sQuadIndices)));

  const size_t indexMemSize = sizeof(sQuadIndices);
  const size_t vertexMemSize = sizeof(sQuadVertices);
  const bgfx_memory_t *vertexMem = bgfx_make_ref(sQuadVertices, vertexMemSize);

  const bgfx_memory_t *indexMem = bgfx_make_ref(sQuadIndices, indexMemSize);

  strcpy(newMesh->header.name, "DEBUG_Quad");

  bgfx_vertex_layout_begin(&newMesh->layout, BGFX_RENDERER_TYPE_NOOP);
  bgfx_vertex_layout_add(&newMesh->layout, BGFX_ATTRIB_POSITION, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_end(&newMesh->layout);

  newMesh->layoutHandle = bgfx_create_vertex_layout(&newMesh->layout);
  newMesh->vbHandle =
      bgfx_create_vertex_buffer(vertexMem, &newMesh->layout, BGFX_BUFFER_NONE);
  newMesh->ibHandle = bgfx_create_index_buffer(indexMem, BGFX_BUFFER_NONE);
}

void DEBUG_DrawQuads(void *meshMem, void *shaderMem, mat4 *model, vec4 *colors,
                     size_t num) {
  bgfx_mesh_instance_state meshInstState;

  bgfx_res_mesh *meshRes = (bgfx_res_mesh *)findBGFXRes(meshMem, "DEBUG_Quad");
  bgfx_res_shader *shaderRes =
      (bgfx_res_shader *)findBGFXRes(shaderMem, "DebugDrawInstanced");

  meshInstState.instanceStride = 80;
  meshInstState.meshState.mesh = meshRes;
  meshInstState.meshState.shader = shaderRes;

  constexpr uint16_t instanceStride = sizeof(mat4) + sizeof(vec4);

  uint32_t drawnUnits =
      bgfx_get_avail_instance_data_buffer(num, meshInstState.instanceStride);

  bgfx_alloc_instance_data_buffer(&meshInstState.idb, drawnUnits,
                                  meshInstState.instanceStride);

  uint8_t *data = meshInstState.idb.data;

  for (size_t i = 0; i < num; ++i) {
    mat4 *mtx = (mat4 *)data;
    glm_mat4_copy(model[i], *mtx);
    float *color = (float *)&data[64];
    color[0] = colors[i][0];
    color[1] = colors[i][1];
    color[2] = colors[i][2];
    color[3] = colors[i][3];
    // glm_rotate(*mtx, time + cubeIdx * 0.37f, rot);
    data += instanceStride;
  }

  // Set vertex and index buffer.
  bgfx_set_vertex_buffer(0, meshRes->vbHandle, 0, UINT32_MAX);
  bgfx_set_index_buffer(meshRes->ibHandle, 0, UINT32_MAX);

  bgfx_set_state(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                     BGFX_STATE_MSAA | BGFX_STATE_PT_LINES,
                 0);

  // Set instance data buffer.
  bgfx_set_instance_data_buffer(&meshInstState.idb, 0, UINT32_MAX);

  // Submit primitive for rendering to view 0.
  bgfx_submit(0, shaderRes->programHandle, 0, BGFX_DISCARD_ALL);
  // bgfx::submit(0, rectangle.program);
}

void debugDraw_drawQuad(void *meshMem, void *materialMem, void *shaderMem,
                        vec3 position, vec4 color, float sizeX, float sizeY) {
  // bgfx_res_mesh *meshRes = (bgfx_res_mesh *)findBGFXRes(meshMem,
  // "Plane:Plane"); bgfx_res_material *materialRes = (bgfx_res_material
  // *)materialMem; bgfx_res_shader *shaderRes =
  //     (bgfx_res_shader *)findBGFXRes(shaderMem, "DebugDraw");

  // mat4 m = GLM_MAT4_IDENTITY_INIT;
  // glm_translate_make(m, position);
  // // glm_mat4_scale(m, sizeX);

  // // Set vertex and index buffer.
  // bgfx_set_transform(m, 1);
  // bgfx_set_uniform(rectangle.colorH, color, 1);
  // bgfx_set_vertex_buffer(0, rectangle.vbh, 0, UINT32_MAX);
  // bgfx_set_index_buffer(meshRes->ibHandle, 0, UINT32_MAX);

  // bgfx_set_state(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
  //                    BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
  //                    BGFX_STATE_MSAA | BGFX_STATE_PT_LINES,
  //                0);

  // // Submit primitive for rendering to view 0.
  // bgfx_submit(0, shaderRes->programHandle, 0, BGFX_DISCARD_ALL);
  // bgfx::submit(0, rectangle.program);
}

void debugDraw_Clean() {
  // bgfx::destroy(rectangle.vbh);
  // bgfx::destroy(rectangle.ibh);
  // bgfx::destroy(rectangle.fsCube);
  // bgfx::destroy(rectangle.vsCube);
  // bgfx::destroy(rectangle.program);
}
