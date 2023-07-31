#include "../rotten_platform.h"
#include "../core/rotten_renderer.h"

#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include "../ext/nuklear.h"

b32 isOpen = false;

struct vec2 {
  f32 x;
  f32 y;
} vec2;

static const char* vertex_shader =
    "#version 330\n"
    "in vec2 position;\n"
    "in vec2 texcoord0;\n"
    "in vec4 color0;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "uniform vec2 disp_size;\n"
    "void main() {\n"
    "   gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
    "   uv = texcoord0;\n"
    "   color = color0;\n"
    "}\n";

static const char* fragment_shader =
    "#version 330\n"
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main(){\n"
    "   frag_color = color * texture(tex, uv.st);\n"
    "}\n";

typedef struct ui_state {
  vertex_buffer_handle vertexBufferHandle;
  index_buffer_handle indexBufferHandle;
  image_handle fontHandle;
  image_handle defImageHandle;
  sampler_handle fontSamplerHandle;
  sampler_handle defImageSamplerHandle;
  shader_program_handle programHandle;
  shader_program_pipeline_handle pipelineHandle;
  struct nk_context ctx;
  struct nk_font_atlas atlas;
  struct nk_allocator alloc;
  b32 initialized;
} ui_state;

memory_arena* temp;
ui_state *ui = 0;

static void* _alloc(nk_handle _unused, void *_old, long unsigned int size)
{
  return memArena_alloc(temp, size);
}

static void _free(nk_handle _unused, void *ptr)
{
  // Temp arena will be cleared after frame
}

static loggerPtr LOG;
#define ASSERT(cond) ASSERT_(cond, #cond, __FUNCTION__, __LINE__, __FILE__)
static assertPtr ASSERT_;

typedef struct nk_vertex {
  f32 pos[2];
  f32 uv[2];
  u8 col[4];
} nk_vertex;

#define MAX_VERTICES 65536
#define VERTEX_DATA_SIZE MAX_VERTICES * sizeof(nk_vertex)
#define INDEX_DATA_SIZE MAX_VERTICES * 3 * sizeof(u16)

struct nk_context* rt_nuklear_alloc(memory_arena* permArena, memory_arena* tempArena) {
  ui = pushType(permArena, ui_state);
  temp = tempArena;
  return &ui->ctx;
}

struct nk_context* rt_nuklear_setup(memory_arena* permanentArea,
                                    memory_arena* tempArena,
				    renderer_command_buffer* buffer,
				    loggerPtr log,
				    assertPtr assert) {
  LOG = log;
  ASSERT_ = assert;

  ui->alloc.alloc = _alloc;
  ui->alloc.free = _free;

  if (ui->initialized) {
    nk_free(&ui->ctx);
    // nk_zero(&ui->ctx.input, sizeof(ui->ctx.input));
    // nk_zero(&ui->ctx.style, sizeof(ui->ctx.style));
    // nk_zero(&ui->ctx.memory, sizeof(ui->ctx.memory));
  }

  nk_font_atlas_init(&ui->atlas, &ui->alloc);
  nk_font_atlas_begin(&ui->atlas);
  int font_width = 0, font_height = 0;
  const void* pixels = nk_font_atlas_bake(&ui->atlas, &font_width, &font_height,
                                          NK_FONT_ATLAS_RGBA32);
  ASSERT((font_width > 0) && (font_height > 0));
  if(!ui->fontHandle){
    renderer_command_create_image* cmd =
        renderer_pushCommand(buffer, renderer_command_create_image);
    cmd->imageHandle = &ui->fontHandle;
    cmd->components = 4;
    cmd->depth = 8;
    cmd->width = font_width;
    cmd->height = font_height;
    cmd->pixels = (void*)pixels;
    cmd->dataSize = (usize)(font_width * font_height) * sizeof(u32);
  }

  nk_font_atlas_end(&ui->atlas, nk_handle_ptr(&ui->fontHandle), 0);
  nk_font_atlas_cleanup(&ui->atlas);
  if (ui->atlas.default_font) {
    nk_style_set_font(&ui->ctx, &ui->atlas.default_font->handle);
  }
  if(!ui->defImageHandle){
    renderer_command_create_image* cmd =
        renderer_pushCommand(buffer, renderer_command_create_image);
    static uint32_t defPixels[64];
    memset(defPixels, 0xFF, sizeof(defPixels));
    cmd->imageHandle = &ui->defImageHandle;
    cmd->components = 4;
    cmd->depth = 8;
    cmd->width = 8;
    cmd->height = 8;
    cmd->pixels = (void*)defPixels;
    cmd->dataSize = (usize)(8 * 8) * sizeof(u32);
  }

  nk_bool init_res = nk_init_fixed(&ui->ctx,
                                   memArena_alloc(permanentArea, MEGABYTES(1)),
                                   MEGABYTES(1), &ui->atlas.default_font->handle);
  ASSERT(init_res);

  if (!ui->vertexBufferHandle){
    renderer_command_create_vertex_buffer* cmd =
        renderer_pushCommand(buffer, renderer_command_create_vertex_buffer);

    cmd->vertexDataSize = VERTEX_DATA_SIZE;
    cmd->indexDataSize = INDEX_DATA_SIZE;
    cmd->isStreamData = true;
    cmd->vertexBufHandle = &ui->vertexBufferHandle;
    cmd->indexBufHandle = &ui->indexBufferHandle;
  }
  if (!ui->fontSamplerHandle){
    renderer_command_create_sampler* cmd =
        renderer_pushCommand(buffer, renderer_command_create_sampler);

    cmd->samplerHandle = &ui->fontSamplerHandle;
  }
  if (!ui->defImageSamplerHandle){
    renderer_command_create_sampler* cmd =
        renderer_pushCommand(buffer, renderer_command_create_sampler);

    cmd->samplerHandle = &ui->defImageSamplerHandle;
  }

  if(!ui->pipelineHandle){
    renderer_command_create_program_pipeline* cmd =
        renderer_pushCommand(buffer, renderer_command_create_program_pipeline);
    cmd->vertexShaderData = (char*)vertex_shader;
    cmd->fragmentShaderData = (char*)fragment_shader;

    cmd->vertexUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 1);
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, vec2, "disp_size");

    cmd->fragmentSamplerEntries = rt_renderer_allocSamplerBuffer(tempArena, 1);
    rt_renderer_pushSamplerEntry(&cmd->fragmentSamplerEntries, "tex");

    cmd->layout = (rt_renderer_data_layout){
        .position =
            {
                .format = vertex_format_float2,
                .offset = NK_OFFSETOF(nk_vertex, pos),
            },
	.normals = {.layoutIdx = -1},
        .uv =
            {
                .format = vertex_format_float2,
                .offset = NK_OFFSETOF(nk_vertex, uv),
            },
        .color =
            {
                .format = vertex_format_ubyte4n,
                .offset = NK_OFFSETOF(nk_vertex, col),
            },
    };
    cmd->cw = true;
    cmd->u16IndexType = true;
    cmd->shaderProgramHandle = &ui->programHandle;
    cmd->progPipelineHandle = &ui->pipelineHandle;
  }
  ui->initialized = true;
  return &ui->ctx;
}

struct nk_context* rt_nuklear_draw(memory_arena* tempArena,
                                   renderer_command_buffer* buffer,
                                   platform_api* api) {
  static const struct nk_draw_vertex_layout_element vertex_layout[] = {
    {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_vertex, pos)},
    {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_vertex, uv)},
    {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_vertex, col)},
    {NK_VERTEX_LAYOUT_END}
  };

  struct nk_convert_config cfg = {0};
  cfg.shape_AA = NK_ANTI_ALIASING_ON;
  cfg.line_AA = NK_ANTI_ALIASING_ON;
  cfg.vertex_layout = vertex_layout;
  cfg.vertex_size = sizeof(nk_vertex);
  cfg.vertex_alignment = 4;
  cfg.circle_segment_count = 22;
  cfg.curve_segment_count = 22;
  cfg.arc_segment_count = 22;
  cfg.global_alpha = 1.0f;


  struct nk_buffer vbuf, ebuf, cmds;

  struct nk_allocator alloc = {0};
  alloc.userdata.ptr = 0;
  alloc.alloc = _alloc;
  alloc.free = _free;

  nk_buffer_init(&vbuf, &alloc, NK_BUFFER_DEFAULT_INITIAL_SIZE);
  nk_buffer_init(&ebuf, &alloc, NK_BUFFER_DEFAULT_INITIAL_SIZE);
  nk_buffer_init(&cmds, &alloc, NK_BUFFER_DEFAULT_INITIAL_SIZE);
  nk_convert(&ui->ctx, &cmds, &vbuf, &ebuf, &cfg);

  const b32 vertex_buffer_overflow = nk_buffer_total(&vbuf) > VERTEX_DATA_SIZE;
  const b32 index_buffer_overflow = nk_buffer_total(&ebuf) > INDEX_DATA_SIZE;
  ASSERT(!vertex_buffer_overflow && !index_buffer_overflow);

  {
    renderer_command_update_vertex_buffer* cmd =
        renderer_pushCommand(buffer, renderer_command_update_vertex_buffer);
    cmd->indexBufHandle = ui->indexBufferHandle;
    cmd->vertexBufHandle = ui->vertexBufferHandle;

    cmd->vertexData = nk_buffer_memory_const(&vbuf);
    cmd->vertexDataSize = nk_buffer_total(&vbuf);

    cmd->indexData = nk_buffer_memory_const(&ebuf);
    cmd->indexDataSize = nk_buffer_total(&ebuf);
  }

  {
    renderer_command_apply_program_pipeline *cmd = renderer_pushCommand(
          buffer, renderer_command_apply_program_pipeline);
      cmd->pipelineHandle = ui->pipelineHandle;
  }
  {
      renderer_command_apply_uniforms* cmd =
          renderer_pushCommand(buffer, renderer_command_apply_uniforms);

      cmd->vertexUniformDataBuffer = rt_renderer_allocUniformDataBuffer(tempArena, sizeof(f32[2]));
      f32* vsUniform = (f32*)cmd->vertexUniformDataBuffer.buffer;
      vsUniform[0] = 1920;
      vsUniform[1] = 1080;
      cmd->vertexUniformDataBuffer.bufferSize = sizeof(f32) * 2;
  }

  const struct nk_draw_command* nk_cmd = NULL;
  u16 indexOffset = 0;
  nk_draw_foreach(nk_cmd, &ui->ctx, &cmds) {
      if (!nk_cmd->elem_count) continue;
      {
        renderer_command_apply_bindings* cmd =
            renderer_pushCommand(buffer, renderer_command_apply_bindings);
        cmd->vertexBufferHandle = ui->vertexBufferHandle;
        cmd->indexBufferHandle = ui->indexBufferHandle;
        cmd->bufferLayout = (vertex_buffer_offset){
	  .position = 0, .normals = 0, .uv = 0, .color = 0, .index = indexOffset};
	cmd->fragmentSamplerBindings = rt_renderer_allocSamplerBindingBuffer(tempArena, 4);

        if (nk_cmd->texture.ptr) {
	  image_handle *handle =(u32*)nk_cmd->texture.ptr;
          rt_renderer_pushSamplerBindingEntry(&cmd->fragmentSamplerBindings,
                                              ui->fontSamplerHandle,
                                              *handle);
        } else {
          rt_renderer_pushSamplerBindingEntry(&cmd->fragmentSamplerBindings,
                                              ui->defImageSamplerHandle,
                                              ui->defImageHandle);
        }
      }
      {
        renderer_command_draw_elements* cmd =
            renderer_pushCommand(buffer, renderer_command_draw_elements);
        cmd->baseElement = 0;
        cmd->numElement = nk_cmd->elem_count;
        cmd->numInstance = 1;
      }
      indexOffset += nk_cmd->elem_count * sizeof(u16);
  }

  nk_buffer_free(&cmds);
  nk_buffer_free(&vbuf);
  nk_buffer_free(&ebuf);
  nk_clear(&ui->ctx);
  return &ui->ctx;
}

void rt_nuklear_handle_input(rt_input_event* inputEventBuf) {
  nk_input_begin(&ui->ctx);
  for (u32 i = 0; i < INPUT_BUFFER_SIZE; i++) {
    rt_input_event* input = inputEventBuf + i;
    if (input->type == RT_INPUT_TYPE_NONE) {
      break;
    }
    // if (input->type == RT_INPUT_TYPE_KEY) {
    //   nk_input_button(struct nk_context *, enum nk_buttons, int x, int y, nk_bool down)
    // }
    else if (input->type == RT_INPUT_TYPE_MOUSE) {
      rt_mouse_event mouse = input->mouse;
      if (mouse.buttonIdx) {
	nk_input_button(&ui->ctx, (nk_buttons)(mouse.buttonIdx - 1),
			mouse.position[0], mouse.position[1],
			mouse.pressed);
      }
      else {
        nk_input_motion(&ui->ctx,
                        mouse.position[0], mouse.position[1]);
      }
    }
  }
  nk_input_end(&ui->ctx);
}
