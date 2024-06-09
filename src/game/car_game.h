#pragma once
#include <bits/types/time_t.h>

#include "physics.h"

#include "../rotten_platform.h"
#include "./shapes.h"

#include "cglm/struct.h"
#include "cglm/types-struct.h"

#define arrayLen(arr) sizeof(arr) / sizeof(arr[0])

#define GLM_VEC4_TO_IVEC4(src) \
  { (int)src[0], (int)src[1], (int)src[2], (int)src[3] }

#define GLM_IVEC4_TO_VEC4(src) \
  { (f32)src[0], (f32)src[1], (f32)src[2], (f32)src[3] }

#define GLM_RGBA_SETP(r, g, b, a, dest) GLM_VEC4_SETP(r, g, b, a, dest)

#define SIGNF(a) a < 0.0f ? -1.0f : 1.0f

static loggerPtr LOG;
#define ASSERT(cond) ASSERT_(cond, #cond, __FUNCTION__, __LINE__, __FILE__)
static assertPtr ASSERT_;

static void* (*MALLOC)(usize size);
static void* (*REALLOC)(void *p, usize newSize);
static void (*FREE)(void *p);

struct memory_arena;

#define loadFile(arena, path, binary, data, fileSize) {		    \
    *fileSize = platformApi->diskIOReadFileSize(path, binary);	    \
    data = memArena_alloc(arena, *fileSize + 1);		    \
    platformApi->diskIOReadFileTo(path, *fileSize, true, data);	    \
  }

#define copyType(from, to, type) memcpy(to, from, sizeof(type))

inline void splitFilePath(const char *filePath, char *path, char *fileName) {
  const char *lastSeparator = strrchr(filePath, '/');

  if (lastSeparator != NULL) {
    // Calculate the length of the path part
    u32 pathLength = lastSeparator - filePath + 1;

    // Copy the path part to the 'path' variable
    strncpy(path, filePath, pathLength);
    path[pathLength] = '\0';  // Null-terminate the path

    // Copy the file name part to the 'fileName' variable
    strcpy(fileName, lastSeparator + 1);
  } else {
    // No separator found, the whole input is the file name
    strcpy(path, "");
    strcpy(fileName, filePath);
  }
}

// #define stringCopy(from, to) stringCopy_(from, to, sizeof(to))
// inline void stringCopy_(const char *from, char *to, u32 maxLen) {
//   u32 len = strlen(from) + 1;
//   ASSERT((len) < maxLen);
//   memcpy(to, from, len);
// }

#define setButtonState(b) b ? \
  BUTTON_PRESSED : BUTTON_RELEASED

typedef enum button_state {
  BUTTON_UP = 0,
  BUTTON_RELEASED = 1,
  BUTTON_PRESSED = 2,
  BUTTON_HELD = 3
} button_state;

typedef struct mouse_state {
  button_state button[3];
  u32 position[2];
  i32 movement[2];
  i32 wheel;
} mouse_state;

typedef struct input_state {
  union {
    struct {
      button_state moveUp;
      button_state moveDown;
      button_state moveLeft;
      button_state moveRight;
      button_state camMoveUp;
      button_state camMoveDown;
      button_state camMoveLeft;
      button_state camMoveRight;
      button_state reset;
      button_state quit;
    };
    button_state buttons[11];
  };
  mouse_state mouse;
} input_state;

// Camera
typedef struct camera_state {
  vec3s position;
  vec3s front;
  vec3s right;
  f32 yaw;
  f32 pitch;
  u32 fov;
} camera_state;

typedef struct model_mesh_data {
  mat4s transform;
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
  vertex_buffer_offset vertexBufferLayout;
  u32 elementNum;
  u32 childNum;
  b32 inWorldSpace;
} model_mesh_data;

typedef struct model_material_data {
  image_handle textureHandle;
  sampler_handle samplerHandle;
  vec4s baseColorValue;
} mode_material_data;

typedef struct terrain_object {
  struct {
    vertex_buffer_handle vertexBufferHandle;
    index_buffer_handle indexBufferHandle;
    vertex_buffer_offset vertexBufferLayout;
    image_handle terrainTexHandle;
    image_handle heightMapTexHandle;
    sampler_handle terrainSamplerHandle;
    sampler_handle heightMapSamplerHandle;
    shader_program_pipeline_handle pipelineHandle;
    shader_program_handle programHandle;
    u32 elementNum;
    u32 vsUniformSize;
  } terrain_model;
  struct {
    vertex_buffer_handle vertexBufferHandle;
    index_buffer_handle indexBufferHandle;
    vertex_buffer_offset vertexBufferLayout;
    shader_program_pipeline_handle pipelineHandle;
    shader_program_handle programHandle;
    u32 elementNum;
  } geometry_model;
  img_data heightMapImg;
  vec4s *geometry;
} terrain_object;

typedef struct skybox_object {
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
  vertex_buffer_offset vertexBufferLayout;
  image_handle texHandle;
  sampler_handle samplerHandle;
  shader_program_pipeline_handle pipelineHandle;
  shader_program_handle programHandle;
  u32 elementNum;
  u32 vsUniformSize;
} skybox_object;

typedef struct car_object {
  struct {
    model_mesh_data meshData[32];
    model_material_data matData[32];

    shader_program_handle programHandle;
    shader_program_pipeline_handle pipelineHandle;
    mat4s transform;
    u32 meshNum;
  } body_model;

  union {
    rigid_body bodies[5];
    struct {
      rigid_body chassis;
      rigid_body wheels[4];
    };
  } body;
  shape bodyShapes[12];
  distance_joint wheelSuspension[4];
  hinge_joint wheelHinge[4];
  contact_manifold manifolds[MAX_CONTACTS];
  f32 turnAngle;
  u32 contactNum;
  union {
    mat4s* t[4];
    struct {
      mat4s* wheelBL;
      mat4s* wheelBR;
      mat4s* wheelFL;
      mat4s* wheelFR;
    };
  } wheel_transforms;
} game_object;

typedef enum draw_state_bit {
  DRAW_CAR                      = 1 << 0,
  DRAW_CAR_COLLIDERS            = 1 << 1,
  DRAW_TERRAIN                  = 1 << 2,
  DRAW_TERRAIN_GEOMETRY         = 1 << 3,
  DRAW_TERRAIN_GEOMETRY_NORMALS = 1 << 4,
} draw_state_bit;

#define isBitSet(drawState, bit) drawState & bit

#define setBit(drawState, bit, set) set ? (drawState | bit) : (drawState & ~bit)

#define toggleBit(drawState, bit) drawState ^ bit

typedef struct deve_state {
  b32 freeCameraView;
  b32 drawDevePanel;
  u16 drawState;
} deve_state;

typedef struct profiler_state {
  f64 totalAcc;
  f64 simulationAcc;
  f64 renderingAcc;
  f64 total;
  f64 simulation;
  f64 rendering;
  f32 elapsedTime;
} profiler_state;

typedef struct game_state {
  b32 initialized;
  input_state input;
  camera_state camera;
  skybox_object skybox;
  terrain_object terrain;
  car_object car;
  time_t assetModTime;
  deve_state deve;
  profiler_state profiler;
} game_state;

static platform_api *platformApi;

img_data importImage(memory_arena *tempArena, const char *imageFilePath, uint8_t channelNum);
shader_data importShader(memory_arena *arena, const char *shaderVsFilePath,
                         const char *shaderFsFilePath);

// f32 getGeometryHeight(vec3s pos, game_state* game, vec3s *normOut);

#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_UINT_DRAW_INDEX
#define NK_INCLUDE_STANDARD_VARARGS

struct nk_context;
nk_context* rt_nuklear_alloc(memory_arena* permArena, memory_arena* tempArena);
nk_context* rt_nuklear_setup(memory_arena* permArena,
                             memory_arena* tempArena,
                             renderer_command_buffer* buffer,
                             loggerPtr log,
                             assertPtr assert);
nk_context* rt_nuklear_draw(memory_arena* tempArena, renderer_command_buffer* buffer, platform_api* api);
void rt_nuklear_handle_input(rt_input_event* inputEventBuf);
