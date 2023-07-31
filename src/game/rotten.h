#pragma once

#include <stdint.h>

#include "../rotten_platform.h"

#define GLM_VEC4_TO_IVEC4(src) \
  { (int)src[0], (int)src[1], (int)src[2], (int)src[3] }

#define GLM_IVEC4_TO_VEC4(src) \
  { (float)src[0], (float)src[1], (float)src[2], (float)src[3] }

#define GLM_VEC2_FROM(src) \
  { src[0], src[1] }

#define GLM_VEC2_SET(v1, v2, dest) \
  dest[0] = v1;                    \
  dest[1] = v2;

#define GLM_VEC4_SETP(v1, v2, v3, v4, dest) \
  {                                         \
    (*dest)[0] = v1;                        \
    (*dest)[1] = v2;                        \
    (*dest)[2] = v3;                        \
    (*dest)[3] = v4;                        \
  }

#define GLM_RGBA_SETP(r, g, b, a, dest) GLM_VEC4_SETP(r, g, b, a, dest)

// Camera
struct camera_state {
  float aspectRatio;
  float moveSpeed;
  vec3 eye;
  vec3 dir;
  vec3 up;
  uint32_t zoomIdx;
};

// // World
struct uniform_grid_params {
  float position[2];
  int cellsNumXY[2];
  float cellSize;
};

#define MAX_ELEMENTS 128 * 128

struct uniform_grid {
  float posX, posY;

  int numCols;
  int numRows;
  int numCells;

  float cellW;
  float cellH;

  float invCellW;
  float invCellH;

  int elements[MAX_ELEMENTS];
};

res_texture *importTexture(platform_state *mem, const char *imageId,
                           const char *filePath);

struct DEBUG {
  bool drawGrid;
  bool pause;
};

struct model_state {
  mesh_buffers meshBuffers;
  material_buffers materialBuffers;
  shader_program shaderProgram;
};

struct game_object {
  model_state model;
};

struct game_state {
  camera_state camera;
  uniform_grid grid;
  game_object terrain;
  game_object cube;
  DEBUG debug;
};
