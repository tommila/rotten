#pragma once
#include "core.h"
#include <string.h>

#include "types.h"
#include "mem.h"

typedef enum renderer_command_type {
  RendCmdType_renderer_command_UNDEFINED = 0,
  RendCmdType_renderer_command_setup,
  RendCmdType_renderer_command_shutdown,
  RendCmdType_renderer_command_free_vertex_buffer,
  RendCmdType_renderer_command_free_program_pipeline,
  RendCmdType_renderer_command_free_sampler,
  RendCmdType_renderer_command_free_image,
  RendCmdType_renderer_command_create_vertex_buffer,
  RendCmdType_renderer_command_create_program_pipeline,
  RendCmdType_renderer_command_create_sampler,
  RendCmdType_renderer_command_create_image,
  RendCmdType_renderer_command_update_vertex_buffer,
  RendCmdType_renderer_command_update_image,
  RendCmdType_renderer_command_apply_bindings,
  RendCmdType_renderer_command_apply_program_pipeline,
  RendCmdType_renderer_command_apply_uniforms,
  RendCmdType_renderer_command_clear,
  RendCmdType_renderer_command_flip,
  RendCmdType_renderer_command_draw_elements,
  RendCmdType_renderer_simple_draw_lines,
  RendCmdType_renderer_simple_draw_box,
  RendCmdType_renderer_simple_draw_sphere,
  RendCmdType_renderer_simple_draw_grid,
  _RendCmdType_renderer_command_num
} renderer_command_type;

// Resources
typedef enum uniform_type {
  uniform_type_invalid,
  uniform_type_f32,
  uniform_type_vec2,
  uniform_type_vec3,
  uniform_type_vec4,
  uniform_type_int,
  uniform_type_ivec2,
  uniform_type_ivec3,
  uniform_type_ivec4,
  uniform_type_mat4
} uniform_type;

typedef enum vertex_format {
  vertex_format_invalid = 0,
  vertex_format_float,
  vertex_format_float2,
  vertex_format_float3,
  vertex_format_float4,
  vertex_format_byte4,
  vertex_format_byte4n,
  vertex_format_ubyte4,
  vertex_format_ubyte4n,
  vertex_format_short2,
  vertex_format_short2n,
  vertex_format_ushort2n,
  vertex_format_short4,
  vertex_format_short4n,
  vertex_format_ushort4n,
  vertex_format_uint10_n2,
  vertex_format_half2,
  vertex_format_half4,
  _vertex_format_num,
} vertex_format;

typedef enum shader_stage_type {
  shader_stage_invalid,
  shader_stage_vertex,
  shader_stage_fragment
} shader_stage_type;

typedef enum primitive_type {
  PRIMITIVE_TRIANGLES,
  PRIMITIVE_LINES,
  PRIMITIVE_NUM
} primitive_type;

typedef u8 layout_attribute_mask;

typedef u32 handle;

typedef handle image_handle;
typedef handle sampler_handle;
typedef handle vertex_buffer_handle;
typedef handle index_buffer_handle;
typedef handle shader_program_handle;
typedef handle shader_program_pipeline_handle;
typedef handle program_pipeline_handle;

typedef struct img_data {
  i16 width;
  i16 height;
  i16 depth;
  i16 components;
  u32 dataSize;
  void* pixels;
} img_data;

typedef struct shader_data {
  void* fsData;
  void* vsData;
} shader_data;

typedef struct uniform_entry {
  uniform_type type;
  char name[32];
} uniform_entry;

typedef struct sampler_entry {
  char name[32];
} sampler_entry;

typedef struct sampler_binding_entry {
  handle textureHandle;
  handle samplerHandle;
} sampler_binding_entry;

typedef struct mesh_data {
  f32* vertexData;
  u32* indices;
  u32 vertexNum;
  u32 colorNum;
  u32 normalNum;
  u32 texCoordNum;
  u32 indexNum;
} mesh_data;

typedef struct material_data {
  char imagePath[64];
  f32 baseColor[4];
} material_data;

typedef struct mesh_buffer_data {
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
} mesh_buffer_data;

typedef struct vertex_buffer_offset {
  u32 position;
  u32 normals;
  u32 uv;
  u32 color;
  u32 index;
} vertex_buffer_offset;

typedef struct pipeline_definition {
  shader_program_handle programHandle;
  layout_attribute_mask layoutAttributeMask;
  primitive_type primitiveType;
  // Add other properties when necessary
} pipeline_definition;

typedef struct rt_renderer_uniform_buffer {
  uniform_entry* buffer;
  u16 num;
  u16 maxNum;
  u32 bufferSize;
} rt_renderer_uniform_buffer;

typedef struct rt_renderer_sampler_buffer {
  sampler_entry* buffer;
  u16 num;
  u16 maxNum;
  u32 bufferSize;
} rt_renderer_sampler_buffer;

typedef struct rt_renderer_sampler_binding_buffer {
  sampler_binding_entry *buffer;
  u16 num;
  u16 maxNum;
} rt_renderer_sampler_binding_buffer;

typedef struct rt_renderer_uniform_data_buffer {
  void *buffer;
  u32 head;
  u32 bufferSize;
  u32 maxSize;
} rt_renderer_uniform_data_buffer;

///////////////////////////////
// Renderer commands structs //
///////////////////////////////

typedef struct renderer_command_header {
  renderer_command_type type;
} renderer_command_header;

typedef struct renderer_command_setup {
  renderer_command_header _header;
  void* (*allocFunc)(usize size);
  void (*freeFunc)(void* ptr);
  void (*log)(LogLevel logLevel, const char* txt, ...);
  void (*assert)(b32 cond, const char* condText, const char* function,
                 int linenum, const char* filename);
} renderer_command_setup;

typedef struct renderer_command_shutdown {
  renderer_command_header _header;
} renderer_command_shutdown;

typedef struct renderer_command_free_vertex_buffer {
  renderer_command_header _header;
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
} renderer_command_free_vertex_buffer;

typedef struct renderer_command_free_program_pipeline {
  renderer_command_header _header;
  shader_program_handle shaderProgramHandle;
  shader_program_pipeline_handle pipelineHandle;
} renderer_command_free_program_pipeline;

typedef struct renderer_command_free_sampler {
  renderer_command_header _header;
  sampler_handle samplerHandle;
} renderer_command_free_sampler;

typedef struct renderer_command_free_image {
  renderer_command_header _header;
  image_handle imageHandle;
} renderer_command_free_image;

typedef struct renderer_command_clear {
  renderer_command_header _header;
  f32 clearColor[4];
  u32 width;
  u32 height;
} renderer_command_clear;

typedef struct renderer_command_flip {
  renderer_command_header _header;
  u32 width;
  u32 height;
} renderer_command_flip;

typedef struct renderer_command_create_vertex_buffer {
  renderer_command_header _header;
  const void* vertexData;
  const void* indexData;
  u32 vertexDataSize;
  u32 indexDataSize;
  vertex_buffer_handle* vertexBufHandle;
  index_buffer_handle* indexBufHandle;
  b32 isStreamData;
} renderer_command_create_vertex_buffer;

typedef struct renderer_command_update_vertex_buffer {
  renderer_command_header _header;
  const void* vertexData;
  const void* indexData;
  u32 vertexDataSize;
  u32 indexDataSize;
  vertex_buffer_handle vertexBufHandle;
  index_buffer_handle indexBufHandle;
} renderer_command_update_vertex_buffer;

typedef struct renderer_command_draw_elements {
  renderer_command_header _header;
  i32 baseElement;
  i32 numElement;
  i32 numInstance;
} renderer_command_draw_elements;

typedef struct attributes {
  vertex_format format;
  i32 layoutIdx;
  i32 offset;
} attributes;

typedef struct rt_renderer_data_layout {
  attributes position;
  attributes normals;
  attributes uv;
  attributes color;
} rt_renderer_data_layout;

typedef struct renderer_command_create_program_pipeline {
  renderer_command_header _header;
  char* fragmentShaderData;
  char* vertexShaderData;
  rt_renderer_uniform_buffer vertexUniformEntries;
  rt_renderer_uniform_buffer fragmentUniformEntries;
  rt_renderer_sampler_buffer vertexSamplerEntries;
  rt_renderer_sampler_buffer fragmentSamplerEntries;
  rt_renderer_data_layout layout;
  primitive_type primitiveType;
  b32 depthTest;
  b32 cw;
  b32 u16IndexType;
  shader_program_handle* shaderProgramHandle;
  program_pipeline_handle* progPipelineHandle;
} renderer_command_create_program_pipeline;

typedef struct renderer_command_create_image {
  renderer_command_header _header;
  i16 width;
  i16 height;
  i16 depth;
  i16 components;
  u32 dataSize;
  void* pixels;
  image_handle* imageHandle;
} renderer_command_create_image;

typedef struct renderer_command_update_image {
  renderer_command_header _header;
  u32 dataSize;
  void* pixels;
  image_handle imageHandle;
} renderer_command_update_image;

typedef struct renderer_command_create_sampler {
  renderer_command_header _header;
  sampler_handle *samplerHandle;
} renderer_command_create_sampler;

typedef struct renderer_command_apply_program_pipeline {
  renderer_command_header _header;
  program_pipeline_handle pipelineHandle;
} renderer_command_apply_program_pipeline;

typedef struct renderer_command_apply_bindings {
  renderer_command_header _header;
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
  vertex_buffer_offset bufferLayout;
  rt_renderer_sampler_binding_buffer vertexSamplerBindings;
  rt_renderer_sampler_binding_buffer fragmentSamplerBindings;
} renderer_command_apply_bindings;

typedef struct renderer_command_apply_uniforms {
  renderer_command_header _header;
  rt_renderer_uniform_data_buffer vertexUniformDataBuffer;
  rt_renderer_uniform_data_buffer fragmentUniformDataBuffer;
} renderer_command_apply_uniforms;

typedef struct renderer_command_buffer {
  memory_arena arena;
} renderer_command_buffer;

//////////////////////////////////
// Renderer simple draw structs //
//////////////////////////////////

typedef struct renderer_simple_draw_box {
  renderer_command_header _header;
  f32 min[3], max[3];
  f32 color[4];
  f32 mvp[16];
} renderer_simple_draw_box;

typedef struct renderer_simple_draw_sphere {
  renderer_command_header _header;
  f32 radius;
  i32 stackCount;
  i32 sectorCount;
  f32 color[4];
  f32 mvp[16];
} renderer_simple_draw_sphere;

typedef struct renderer_simple_draw_lines {
  renderer_command_header _header;
  f32 *lines;
  u32 lineNum;
  f32 color[4];
  f32 mvp[16];
} renderer_simple_draw_lines;

#define renderer_pushCommandArray(buffer, type, num)                    \
  (type*) _renderer_pushCommand(buffer, RendCmdType_##type, sizeof(type), num)

#define renderer_pushCommand(buffer, type) \
  (type*) _renderer_pushCommand(buffer, RendCmdType_##type, sizeof(type), 1)

static inline renderer_command_header* _renderer_pushCommand(
    renderer_command_buffer* buffer, renderer_command_type type, usize size,
    u32 num) {

  renderer_command_header* cmd =
      (renderer_command_header*)memArena_allocUnalign(&buffer->arena,
                                                      size * num);
  memset(cmd, 0, size * num);
  for (u32 i = 0; i < num; i++) {
    renderer_command_header* it = (renderer_command_header*)((char*)cmd + size * i);
    it->type = type;
  }
  return cmd;
}

static inline rt_renderer_uniform_buffer rt_renderer_allocUniformBuffer(memory_arena* arena,
                                                          u16 maxNum) {
  rt_renderer_uniform_buffer buffer = {.buffer = 0, .num = 0, .maxNum = maxNum};
  buffer.buffer = (uniform_entry*)memArena_alloc(arena, sizeof(uniform_entry) * maxNum);
  return buffer;
}

static inline rt_renderer_sampler_buffer rt_renderer_allocSamplerBuffer(memory_arena* arena,
                                                          u16 maxNum) {
  rt_renderer_sampler_buffer buffer = {.buffer = 0, .num = 0, .maxNum = maxNum};
  buffer.buffer = (sampler_entry*)memArena_alloc(arena, sizeof(uniform_entry) * maxNum);
  return buffer;
}

static inline rt_renderer_sampler_binding_buffer rt_renderer_allocSamplerBindingBuffer(memory_arena* arena,
                                                          u16 maxNum) {
  rt_renderer_sampler_binding_buffer buffer = {.buffer = 0, .num = 0, .maxNum = maxNum};
  buffer.buffer = (sampler_binding_entry*)memArena_alloc(arena, sizeof(uniform_entry) * maxNum);
  return buffer;
}

static inline rt_renderer_uniform_data_buffer rt_renderer_allocUniformDataBuffer(memory_arena* arena,
                                                          u32 maxSize) {
  rt_renderer_uniform_data_buffer buffer = {.buffer = 0, .bufferSize = 0, .maxSize = maxSize};
  buffer.buffer = memArena_alloc(arena, maxSize);
  return buffer;
}

#define rt_renderer_pushUniformEntry(buff, type, name) \
  _pushUniformEntry(buff, uniform_type_##type, name, sizeof(type))

inline uniform_entry* _pushUniformEntry(rt_renderer_uniform_buffer* buff,
                                       uniform_type type, const char* name,
                                       u32 size) {
  uniform_entry *entry = buff->buffer + buff->num;
  entry->type = type;
  stringCopy(name, entry->name);
  buff->bufferSize += size;
  buff->num++;
  return entry;
}

#define rt_renderer_pushSamplerEntry(buff, name) \
  _pushSamplerEntry(buff, name)

inline sampler_entry* _pushSamplerEntry(rt_renderer_sampler_buffer* buff,
					const char* name) {
  sampler_entry *entry = buff->buffer + buff->num;
  stringCopy(name, entry->name);
  buff->bufferSize += sizeof(entry->name);
  buff->num++;
  return entry;
}

#define rt_renderer_pushSamplerBindingEntry(buff, sampHandle, texHandle) \
  _pushSamplerBindingEntry(buff, sampHandle, texHandle)

inline sampler_binding_entry* _pushSamplerBindingEntry(rt_renderer_sampler_binding_buffer* buff,
						       sampler_handle samp, image_handle tex) {
  sampler_binding_entry *entry = buff->buffer + buff->num;
  entry->samplerHandle = samp;
  entry->textureHandle = tex;
  buff->num++;
  return entry;
}

#define rt_renderer_pushUniformDataEntry(buff, type) \
  (type*)_pushUniformDataEntry(buff, sizeof(type))

inline void* _pushUniformDataEntry(rt_renderer_uniform_data_buffer* buff, u32 size) {
  void *entry = (u32*)buff->buffer + buff->head;
  buff->head += size;
  buff->bufferSize += size;
  return entry;
}

#define RT_RENDERER_FLUSH_BUFFER(name) \
  void name(renderer_command_buffer* buffer)
typedef RT_RENDERER_FLUSH_BUFFER(rt_renderer_flushCommandBuffer);
