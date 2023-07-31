#pragma once

#include <bits/types/time_t.h>
#include <stdio.h>
#include <time.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "cglm/types.h"

#define CONCAT2(dest, str1, str2) \
  strcpy(dest, str1);             \
  strcat(dest, str2);             \
  dest[strlen(str1) + strlen(str2)] = '\0'
#define CONCAT3(dest, str1, str2, str3) \
  strcpy(dest, str1);                   \
  strcat(strcat(dest, str2), str3);     \
  dest[strlen(str1) + strlen(str2) + strlen(str3)] = '\0'
#define CONCAT4(dest, str1, str2, str3, str4)     \
  strcpy(dest, str1);                             \
  strcat(strcat(strcat(dest, str2), str3), str4); \
  dest[strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4)] = '\0'
#define CONCAT5(dest, str1, str2, str3, str4, str5)                \
  strcpy(dest, str1);                                              \
  strcat(strcat(strcat(strcat(dest, str2), str3), str4), str5);    \
  dest[strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + \
       strlen(str5)] = '\0'

#define ARR_LEN(arr) sizeof(arr) / sizeof(arr[0])

enum LogLevel {
  LOG_LEVEL_VERBOSE = 1,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_CRITICAL,
  LOG_LEVELS
};

#define LOG_ LOG
static void (*LOG)(LogLevel, const char*, ...);

#define ASSERT(cond) ASSERT_(cond, #cond, __FUNCTION__, __LINE__, __FILE__)
static void (*ASSERT_)(bool, const char*, const char*, int, const char*);

#define SAFE_FREE(p) \
  if (p) free(p)

#define stringCopy(from, to) stringCopy_(from, to, sizeof(to));
inline void stringCopy_(const char* from, char* to, size_t maxLen) {
  size_t len = strlen(from) + 1;
  ASSERT((len) < maxLen);
  memcpy(to, from, len);
}

inline void splitFilePath(const char* filePath, char* path, char* fileName) {
  const char* lastSeparator = strrchr(filePath, '/');

  if (lastSeparator != NULL) {
    // Calculate the length of the path part
    size_t pathLength = lastSeparator - filePath + 1;

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

#define KILOBYTES(val) ((val)*1024)
#define MEGABYTES(val) (KILOBYTES(val) * 1024)
#define GIGABYTES(val) (MEGABYTES(val) * 1024)

struct memory_block {
  void* data;
  size_t head;
  size_t size;
};

struct game_memory_area {
  memory_block texMemBlock;
  memory_block matMemBlock;
  memory_block meshMemBlock;
  memory_block modelMemBlock;
  memory_block shaderMemBlock;
  memory_block gameMemBlock;
};

// TODO: update input handling
enum button_state { Up = 0, Released = 1, Pressed = 2, Held = 3 };

struct button {
  int32_t sym;
  button_state state;
};

struct input_state {
  float axis_state[2];
  uint32_t wheel;
  union {
    button allButtons[8];
    struct {
      button quitButton;
      button pauseButton;
      button gridButton;
      button timeStepButton;
      button leftButton;
      button rightButton;
      button upButton;
      button downButton;
    };
  };
};

// Resources
enum res_type { texture, material, mesh, model, shader, num };

typedef struct file_info {
  char path[128];
  time_t modTime;
} file_info;

typedef struct res_header {
  char name[64];
  res_type type;
} res_header;

typedef struct img_data {
  void* pixels;
  void* context;
  size_t width;
  size_t height;
  int depth;
  int components;
  size_t dataSize;
  file_info fileInfo;
} img_data;

typedef struct res_texture {
  res_header header;
  img_data data;
} res_texture;

typedef struct shader_data {
  void* fsData;
  void* vsData;
  size_t fsDataSize;
  size_t vsDataSize;
  file_info fsVileInfo;
  file_info vsFileInfo;
} shader_data;

typedef struct res_shader {
  res_header header;
  shader_data data;
} res_shader;

typedef struct mesh_data {
  float* vertices;
  float* texCoords;
  float* colors;
  uint16_t* indices;
  size_t vertexNum;
  size_t colorNum;
  size_t texCoordNum;
  size_t indexNum;
} mesh_data;

typedef struct res_mesh {
  res_header header;
  mesh_data data;
  mat4 transform;
} res_mesh;

struct res_material {
  res_header header;
  res_texture* texture;
  vec4 baseColor;
};

typedef struct res_model {
  res_header header;
  res_mesh* meshes[64];
  res_material* materials[64];
  size_t meshNum;
  mat4 transform;
  file_info fileInfo;
} res_model;

// Buffers
struct mesh_buffers {
  uint16_t vertexBufferHandle;
  uint16_t indexBufferHandle;
  uint16_t vertexLayoutHandle;
};

struct shader_program {
  uint16_t vertexShaderHandle;
  uint16_t fragmentShaderHandle;
  uint16_t programHandle;
};

struct material_buffers {
  uint16_t textureHandle;
  uint16_t samplerHandle;
  uint16_t baseColorHandle;
  vec4 baseColor;
};

#define pushArray(memory, count, type) \
  (type*)pushSize_(memory, count * sizeof(type))
#define pushSize(memory, type) (type*)pushSize_(memory, sizeof(type))
inline void* pushSize_(memory_block* mem, size_t size) {
  ASSERT((mem->head + size) < mem->size);
  void* it = (uint8_t*)mem->data + mem->head;
  mem->head += size;
  return it;
}

#define findRes(memory, name, type) (type*)findRes_(memory, name, sizeof(type))
inline void* findRes_(memory_block* mem, const char* name, size_t stride) {
  size_t it = 0;
  while (it < mem->head) {
    void* ptr = (uint8_t*)mem->data + it;
    res_header* res = (res_header*)ptr;
    if (strcmp(res->name, name) == 0) {
      return ptr;
    }
    it += stride;
  }
  return 0;
}

struct platform_state;
struct platform_api {
  void (*initDisplay)(uint16_t, uint16_t);
  void (*renderBegin)(mat4*, mat4*);
  void (*renderEnd)();
  void (*renderMesh)(mesh_buffers*, material_buffers*, shader_program*, mat4);
  void (*renderMeshInstanced)(mesh_buffers*, material_buffers*, shader_program*,
                              size_t, mat4*, vec4*);
  bool (*initMeshBuffers)(mesh_data*, mesh_buffers*);
  bool (*initMaterialBuffers)(res_material*, material_buffers*);
  bool (*initShaderProgram)(shader_data*, shader_program*);
  bool (*updateMaterialBuffers)(res_material*, material_buffers*);
  void (*freeMeshBuffers)(mesh_buffers*);
  void (*freeMaterialBuffers)(material_buffers*);
  void (*freeShaderProgram)(shader_program*);

  void (*log)(LogLevel, const char*, ...);
  void (*assert)(bool cond, const char* condText, const char* function,
                 int linenum, const char* filename);
  void* (*diskIOReadFile)(const char* path, size_t* dataSize, bool isBinary);
  time_t (*diskIOReadModTime)(const char* path);
};

struct platform_state {
  memory_block permanentMemory;
  memory_block temporaryMemory;
  game_memory_area memory;
  platform_api api;
};

extern "C" {
// Game code //
#define GAME_ON_START(name) void name(platform_state* platform)
typedef GAME_ON_START(game_on_start);

#define GAME_UPDATE_AND_RENDER(name)                               \
  bool name(float delta, float duration, platform_state* platform, \
            input_state* input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
}

struct game_code {
  void* libGameCode;
  time_t libLastWriteTime;

  game_on_start* onStartFunc;
  game_update_and_render* gameLoopFunc;

  bool isValid;
};
