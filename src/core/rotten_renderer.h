typedef enum rt_render_entry_type {
  RendCmdType_rt_render_entry_UNDEFINED = 0,
  RendCmdType_rt_render_entry_begin,
  RendCmdType_rt_render_entry_shutdown,
  RendCmdType_rt_render_entry_free_vertex_buffer,
  RendCmdType_rt_render_entry_free_program_pipeline,
  RendCmdType_rt_render_entry_free_sampler,
  RendCmdType_rt_render_entry_free_image,
  RendCmdType_rt_render_entry_create_vertex_buffer,
  RendCmdType_rt_render_entry_create_shader_program,
  RendCmdType_rt_render_entry_create_sampler,
  RendCmdType_rt_render_entry_create_image,
  RendCmdType_rt_render_entry_update_vertex_buffer,
  RendCmdType_rt_render_entry_update_image,
  RendCmdType_rt_render_entry_update_shader_program,
  RendCmdType_rt_render_entry_apply_bindings,
  RendCmdType_rt_render_entry_apply_program,
  RendCmdType_rt_render_entry_apply_uniforms,
  RendCmdType_rt_render_entry_clear,
  RendCmdType_rt_render_entry_flip,
  RendCmdType_rt_render_entry_draw_elements,
  RendCmdType_rt_render_simple_lines,
  RendCmdType_rt_render_simple_box,
  RendCmdType_rt_render_simple_sphere,
  RendCmdType_rt_render_simple_grid,
  _RendCmdType_rt_render_entry_num
} rt_render_entry_type;

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

typedef enum rt_renderer_data_type {
  rt_renderer_data_type_i8,
  rt_renderer_data_type_u8,
  rt_renderer_data_type_i16,
  rt_renderer_data_type_u16,
  rt_renderer_data_type_i32,
  rt_renderer_data_type_u32,
  rt_renderer_data_type_f32,
  rt_renderer_data_type_2bytes,
  rt_renderer_data_type_3bytes,
  rt_renderer_data_type_4bytes,
  rt_renderer_data_type_f64,
} rt_renderer_data_type;

typedef enum shader_stage_type {
  shader_stage_invalid,
  shader_stage_vertex,
  shader_stage_fragment
} shader_stage_type;

typedef enum primitive_type {
  rt_primitive_triangles = 0,
  rt_primitive_lines,
} primitive_type;

typedef u8 layout_attribute_mask;

typedef u32 handle;

typedef handle image_handle;
typedef handle sampler_handle;
typedef handle vertex_array_handle;
typedef handle vertex_buffer_handle;
typedef handle index_buffer_handle;
typedef handle shader_program_handle;

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
  void* data;
} uniform_entry;

typedef struct sampler_entry {
  char name[32];
} sampler_entry;

typedef struct sampler_binding_entry {
  handle textureHandle;
  handle samplerHandle;
} sampler_binding_entry;

typedef struct mesh_buffer_data {
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
} mesh_buffer_data;

typedef struct rt_renderer_uniform_buffer {
  uniform_entry* buffer;
  u16 num;
  u16 maxNum;
  //u32 bufferSize;
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

typedef struct rt_render_entry_header {
  rt_render_entry_type type;
  char id[128];
} rt_render_entry_header;

typedef struct rt_render_entry_begin {
  rt_render_entry_header _header;
} rt_render_entry_begin;

typedef struct rt_render_entry_shutdown {
  rt_render_entry_header _header;
} rt_render_entry_shutdown;

typedef struct rt_render_entry_free_vertex_buffer {
  rt_render_entry_header _header;
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
} rt_render_entry_free_vertex_buffer;

typedef struct rt_render_entry_free_program_pipeline {
  rt_render_entry_header _header;
  shader_program_handle shaderProgramHandle;
} rt_render_entry_free_program_pipeline;

typedef struct rt_render_entry_free_sampler {
  rt_render_entry_header _header;
  sampler_handle samplerHandle;
} rt_render_entry_free_sampler;

typedef struct rt_render_entry_free_image {
  rt_render_entry_header _header;
  image_handle imageHandle;
} rt_render_entry_free_image;

typedef struct rt_render_entry_clear {
  rt_render_entry_header _header;
  f32 clearColor[4];
  u32 width;
  u32 height;
} rt_render_entry_clear;

typedef struct rt_render_entry_flip {
  rt_render_entry_header _header;
  u32 width;
  u32 height;
} rt_render_entry_flip;

typedef struct rt_renderer_vertex_attributes {
  i32 count;
  i32 offset;
  i32 stride;
  b32 normalized;
  rt_renderer_data_type type;
} rt_renderer_vertex_attributes;

typedef struct rt_render_entry_create_vertex_buffer {
  rt_render_entry_header _header;
  vertex_array_handle* vertexArrHandle;
  vertex_buffer_handle* vertexBufHandle;
  index_buffer_handle* indexBufHandle;
  const void* vertexData;
  const void* indexData;
  usize vertexDataSize;
  usize indexDataSize;
  rt_renderer_vertex_attributes vertexAttributes[4];
  b32 isStreamData;
} rt_render_entry_create_vertex_buffer;

typedef struct rt_render_entry_update_vertex_buffer {
  rt_render_entry_header _header;
  vertex_buffer_handle vertexBufHandle;
  index_buffer_handle indexBufHandle;
  const void* vertexData;
  const void* indexData;
  u32 vertexDataSize;
  u32 indexDataSize;
  i32 vertexDataOffset;
  i32 indexDataOffset;
} rt_render_entry_update_vertex_buffer;

typedef struct rt_render_entry_draw_elements {
  rt_render_entry_header _header;
  primitive_type mode;
  i32 numElement;
  i32 baseElement;
  i32 baseVertex;
} rt_render_entry_draw_elements;

typedef struct rt_render_entry_create_shader_program {
  rt_render_entry_header _header;
  shader_program_handle* shaderProgramHandle;
  const char* fragmentShaderData;
  const char* vertexShaderData;
} rt_render_entry_create_shader_program;

typedef rt_render_entry_create_shader_program rt_render_entry_update_shader_program;

typedef struct rt_render_entry_create_image {
  rt_render_entry_header _header;
  image_handle* imageHandle;
  i16 width;
  i16 height;
  i16 depth;
  i16 components;
  void* pixels;
} rt_render_entry_create_image;

typedef struct rt_render_entry_update_image {
  rt_render_entry_header _header;
  image_handle imageHandle;
  i16 width;
  i16 height;
  i16 depth;
  i16 components;
  void* pixels;
} rt_render_entry_update_image;

typedef struct rt_render_entry_create_sampler {
  rt_render_entry_header _header;
  sampler_handle *samplerHandle;
} rt_render_entry_create_sampler;

typedef struct rt_render_entry_apply_program {
  rt_render_entry_header _header;
  shader_program_handle programHandle;
  b32 enableDepthTest;
  b32 enableBlending;
  b32 enableCull;
  b32 ccwFrontFace;
} rt_render_entry_apply_program;

typedef struct rt_render_entry_apply_bindings {
  rt_render_entry_header _header;
  vertex_array_handle vertexArrayHandle;
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
  sampler_binding_entry textureBindings[8];
} rt_render_entry_apply_bindings;

typedef struct rt_render_entry_apply_uniforms {
  rt_render_entry_header _header;
  shader_program_handle shaderProgram;
  uniform_entry uniforms[16];
} rt_render_entry_apply_uniforms;

typedef struct rt_render_entry_buffer {
  memory_arena arena;
} rt_render_entry_buffer;

typedef struct rt_render_simple_box {
  rt_render_entry_header _header;
  f32 min[3], max[3];
  f32 color[4];
  f32 mvp[16];
} rt_render_simple_box;

typedef struct rt_render_simple_sphere {
  rt_render_entry_header _header;
  f32 radius;
  i32 stackCount;
  i32 sectorCount;
  f32 color[4];
  f32 mvp[16];
} rt_render_simple_sphere;

typedef struct rt_render_simple_lines {
  rt_render_entry_header _header;
  f32 *lines;
  u32 lineNum;
  f32 lineWidth;
  f32 color[4];
  f32 mvp[16];
} rt_render_simple_lines;

#define rt_renderer_pushEntryArray(buffer, type, num)                    \
  (type*) _rt_renderer_pushEntry(buffer, RendCmdType_##type, sizeof(type), num, __FILE__)

#define rt_renderer_pushEntry(buffer, type) \
  (type*) _rt_renderer_pushEntry(buffer, RendCmdType_##type, sizeof(type), 1, __FILE__)

static inline rt_render_entry_header* _rt_renderer_pushEntry(
    rt_render_entry_buffer* buffer, rt_render_entry_type type, usize size,
    u32 num, const char* id) {

  rt_render_entry_header* cmd =
      (rt_render_entry_header*)memArena_allocUnalign(&buffer->arena,
                                                      size * num);
  memset(cmd, 0, size * num);
  for (u32 i = 0; i < num; i++) {
    rt_render_entry_header* it = (rt_render_entry_header*)((char*)cmd + size * i);
    it->type = type;
    strcpy(it->id, id);
  }
  return cmd;
}

void rendererInit(void (*log)(LogLevel logLevel, const char* txt,
                                         ...),
                             void (*assert)(b32 cond, const char* condText,
                                            const char* function, int linenum,
                                            const char* filename),
                             void* (*glGetProcAddressFunc)(const char* proc));
void flushCommandBuffer(rt_render_entry_buffer* buffer);
#define RT_RENDERER_INIT(name)                                          \
  void name(                                                               \
      void (*log)(LogLevel logLevel, const char* txt, ...),                \
      void (*assert)(b32 cond, const char* condText, const char* function, \
                     int linenum, const char* filename),                   \
      void* (*glGetProcAddressFunc)(const char* proc))
typedef RT_RENDERER_INIT(rt_renderer_init);

#define RT_RENDERER_FLUSH_BUFFER(name) \
  void name(rt_render_entry_buffer* buffer)
typedef RT_RENDERER_FLUSH_BUFFER(rt_renderer_flushCommandBuffer);
