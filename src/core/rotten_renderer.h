typedef enum rt_command_type {
  rt_command_type_UNDEFINED = 0,
  rt_command_type_begin,
  rt_command_type_shutdown,
  rt_command_type_free_vertex_buffer,
  rt_command_type_free_program_pipeline,
  rt_command_type_free_sampler,
  rt_command_type_free_texture,
  rt_command_type_create_vertex_buffer,
  rt_command_type_create_shader_program,
  rt_command_type_create_sampler,
  rt_command_type_create_texture,
  rt_command_type_update_vertex_buffer,
  rt_command_type_update_texture,
  rt_command_type_update_shader_program,
  rt_command_type_apply_bindings,
  rt_command_type_apply_program,
  rt_command_type_apply_uniforms,
  rt_command_type_clear,
  rt_command_type_flip,
  rt_command_type_draw_elements,
  rt_command_type_render_simple_lines,
  rt_command_type_render_simple_box,
  rt_command_type_render_simple_arrow,
  rt_command_type_render_simple_sphere,
  rt_command_type_render_simple_grid,
  _rt_command_type_num
} rt_command_type;

// Resources
typedef enum uniform_type {
  rt_uniform_type_invalid,
  rt_uniform_type_f32,
  rt_uniform_type_vec2,
  rt_uniform_type_vec3,
  rt_uniform_type_vec4,
  rt_uniform_type_int,
  rt_uniform_type_ivec2,
  rt_uniform_type_ivec3,
  rt_uniform_type_ivec4,
  rt_uniform_type_mat4
} rt_uniform_type;

typedef enum rt_renderer_data_type {
  rt_data_type_i8,
  rt_data_type_u8,
  rt_data_type_i16,
  rt_data_type_u16,
  rt_data_type_i32,
  rt_data_type_u32,
  rt_data_type_f32,
  rt_data_type_2bytes,
  rt_data_type_3bytes,
  rt_data_type_4bytes,
  rt_data_type_f64,
} rt_data_type;

typedef enum rt_shader_stage_type {
  rt_shader_stage_invalid,
  rt_shader_stage_vertex,
  rt_shader_stage_fragment
} rt_shader_stage_type;

typedef enum rt_primitive_type {
  rt_primitive_triangles = 0,
  rt_primitive_lines,
} rt_primitive_type;

typedef enum rt_texture_type {
  rt_texture_type_2d,
  rt_texture_type_cubemap
} rt_texture_type;

typedef u32 rt_handle;

typedef rt_handle rt_image_handle;
typedef rt_handle rt_sampler_handle;
typedef rt_handle rt_vertex_array_handle;
typedef rt_handle rt_vertex_buffer_handle;
typedef rt_handle rt_index_buffer_handle;
typedef rt_handle rt_shader_program_handle;

typedef struct rt_shader_data {
  str8 fsData;
  str8 vsData;
} rt_shader_data;

typedef struct rt_image_data {
  i16 width;
  i16 height;
  i16 depth;
  i16 components;
  u32 dataSize;
  void* pixels;
} rt_image_data;

typedef struct rt_uniform_entry {
  rt_uniform_type type;
  str8 name;
  void* data;
} rt_uniform_data;

typedef struct rt_sampler_entry {
  str8 name;
} rt_sampler_data;

typedef struct rt_binding_data {
  rt_handle textureHandle;
  rt_texture_type textureType;
  rt_handle samplerHandle;
} rt_binding_data;

typedef struct rt_mesh_buffer_data {
  rt_vertex_buffer_handle vertexBufferHandle;
  rt_index_buffer_handle indexBufferHandle;
} rt_mesh_buffer_data;

typedef struct rt_renderer_vertex_attributes {
  i32 count;
  i32 offset;
  i32 stride;
  b32 normalized;
  rt_data_type type;
} rt_vertex_attributes;


///////////////////////////////
// Renderer commands structs //
///////////////////////////////

typedef struct rt_command_header {
  rt_command_type type;
  str8 id;
} rt_command_header;

typedef struct rt_command_begin {
  rt_command_header _header;
} rt_command_begin;

typedef struct rt_command_shutdown {
  rt_command_header _header;
} rt_command_shutdown;

typedef struct rt_command_free_vertex_buffer {
  rt_command_header _header;
  rt_vertex_buffer_handle vertexBufferHandle;
  rt_index_buffer_handle indexBufferHandle;
} rt_command_free_vertex_buffer;

typedef struct rt_command_free_program_pipeline {
  rt_command_header _header;
  rt_shader_program_handle shaderProgramHandle;
} rt_command_free_program_pipeline;

typedef struct rt_command_free_sampler {
  rt_command_header _header;
  rt_sampler_handle samplerHandle;
} rt_command_free_sampler;

typedef struct rt_command_free_texture {
  rt_command_header _header;
  rt_image_handle imageHandle;
} rt_command_free_texture;

typedef struct rt_command_clear {
  rt_command_header _header;
  f32 clearColor[4];
  u32 width;
  u32 height;
} rt_command_clear;

typedef struct rt_command_flip {
  rt_command_header _header;
  u32 width;
  u32 height;
} rt_command_flip;

typedef struct rt_command_create_vertex_buffer {
  rt_command_header _header;
  rt_vertex_array_handle* vertexArrHandle;
  rt_vertex_buffer_handle* vertexBufHandle;
  rt_index_buffer_handle* indexBufHandle;
  const void* vertexData;
  const void* indexData;
  usize vertexDataSize;
  usize indexDataSize;
  rt_vertex_attributes vertexAttributes[4];
  b32 isStreamData;
} rt_command_create_vertex_buffer;

typedef struct rt_command_update_vertex_buffer {
  rt_command_header _header;
  rt_vertex_buffer_handle vertexBufHandle;
  rt_index_buffer_handle indexBufHandle;
  const void* vertexData;
  const void* indexData;
  u32 vertexDataSize;
  u32 indexDataSize;
  i32 vertexDataOffset;
  i32 indexDataOffset;
} rt_command_update_vertex_buffer;

typedef struct rt_command_draw_elements {
  rt_command_header _header;
  rt_primitive_type mode;
  f32 lineWidth;
  v4i scissor;
  i32 numElement;
  i32 baseElement;
  i32 baseVertex;
} rt_command_draw_elements;

typedef struct rt_command_create_shader_program {
  rt_command_header _header;
  rt_shader_program_handle* shaderProgramHandle;
  str8 fragmentShaderData;
  str8 vertexShaderData;
} rt_command_create_shader_program;

typedef rt_command_create_shader_program rt_command_update_shader_program;

typedef struct rt_command_create_texture {
  rt_command_header _header;
  rt_texture_type textureType;
  rt_image_handle* imageHandle;
  rt_image_data image[6];
} rt_command_create_texture;

typedef struct rt_command_update_texture {
  rt_command_header _header;
  rt_texture_type textureType;
  rt_image_handle imageHandle;
  rt_image_data image[6];
} rt_command_update_texture;

typedef struct rt_command_create_sampler {
  rt_command_header _header;
  rt_sampler_handle *samplerHandle;
} rt_command_create_sampler;

typedef struct rt_command_apply_program {
  rt_command_header _header;
  rt_shader_program_handle programHandle;
  b32 enableDepthTest;
  b32 enableBlending;
  b32 enableScissorTest;
  b32 enableCull;
  b32 ccwFrontFace;
} rt_command_apply_program;

typedef struct rt_command_apply_bindings {
  rt_command_header _header;
  rt_vertex_array_handle vertexArrayHandle;
  rt_vertex_buffer_handle vertexBufferHandle;
  rt_index_buffer_handle indexBufferHandle;
  rt_binding_data textureBindings[8];
} rt_command_apply_bindings;

typedef struct rt_command_apply_uniforms {
  rt_command_header _header;
  rt_shader_program_handle shaderProgram;
  rt_uniform_data uniforms[16];
} rt_command_apply_uniforms;

typedef struct rt_command_buffer {
  memory_arena arena;
} rt_command_buffer;

typedef struct rt_command_render_simple_box {
  rt_command_header _header;
  v3 min, max;
  v4 color;
  m4x4 projView;
  m4x4 model;
} rt_command_render_simple_box;

typedef struct rt_render_simple_arrow {
  rt_command_header _header;
  f32 length, size;
  v4 color;
  m4x4 projView;
  m4x4 model;
} rt_command_render_simple_arrow;

typedef struct rt_command_render_simple_sphere {
  rt_command_header _header;
  f32 radius;
  i32 stackCount;
  i32 sectorCount;
  v4 color;
  m4x4 projView;
  m4x4 model;
} rt_command_render_simple_sphere;

typedef struct rt_command_render_simple_lines {
  rt_command_header _header;
  v3 *lines;
  u32 lineNum;
  f32 lineWidth;
  v4 color;
  m4x4 projView;
  m4x4 model;
} rt_command_render_simple_lines;
#define rt_pushRenderCommandArray(buffer, type, num)                    \
  (rt_command_##type*) _rt_pushRenderCommand(buffer,  rt_command_type_##type, sizeof(rt_command_##type), num, string8(__FILE__))

#define rt_pushRenderCommand(buffer, type) \
  (rt_command_##type*) _rt_pushRenderCommand(buffer, rt_command_type_##type, sizeof(rt_command_##type), 1, string8(__FILE__))

static inline rt_command_header* _rt_pushRenderCommand(
    rt_command_buffer* buffer, rt_command_type type, usize size,
    u32 num, str8 id) {

  rt_command_header* cmd =
      (rt_command_header*)memArena_allocUnalign(&buffer->arena,
                                                      size * num);
  memset(cmd, 0, size * num);
  for (u32 i = 0; i < num; i++) {
    rt_command_header* it = (rt_command_header*)((char*)cmd + size * i);
    it->type = type;
    it->id = id;
  }
  return cmd;
}

void rendererInit(void (*log)(LogLevel logLevel, const char* txt, ...),
                             void (*assert)(b32 cond, const char* condText,
                                            const char* function, int linenum,
                                            const char* filename),
                             void* (*glGetProcAddressFunc)(const char* proc));
void flushCommandBuffer(rt_command_buffer* buffer);
#define RT_RENDERER_INIT(name)                                          \
  void name(                                                               \
      void (*log)(LogLevel logLevel, const char* txt, ...),                \
      void (*assert)(b32 cond, const char* condText, const char* function, \
                     int linenum, const char* filename),                   \
      void* (*glGetProcAddressFunc)(const char* proc))
typedef RT_RENDERER_INIT(rt_init);

#define RT_RENDERER_FLUSH_BUFFER(name) \
  void name(rt_command_buffer* buffer)
typedef RT_RENDERER_FLUSH_BUFFER(rt_flushCommandBuffer);
