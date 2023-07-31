#include <stdio.h>
#include <math.h>
#include "core.h"
#include "rotten_renderer.h"

#define SOKOL_IMPL
#define SOKOL_GLCORE33

#include "../ext/sokol_gfx.h"

#define PI 3.14159265359f

static sg_shader_desc simpleShaderDesc = {
  .vs = {
    .source = "#version 330\n"
    "layout(location = 0) in vec4 position;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "  gl_Position = mvp * position;\n"
    "}\n",
    .uniform_blocks[0] = {
      .uniforms[0] = {
        .name = "mvp",
	.type = SG_UNIFORMTYPE_MAT4
      },
      .size = sizeof(f32) * 16
    }
  },
  .fs = {
    .source = "#version 330\n"
    "out vec4 frag_color;\n"
    "uniform vec4 u_color;\n"
    "void main() {\n"
    "  frag_color = u_color;\n"
    "}\n",
    .uniform_blocks[0] = {
      .uniforms[0] = {
	.name = "u_color",
	.type = SG_UNIFORMTYPE_FLOAT4
      },
      .size = sizeof(f32) * 4
    }
  },
};

static inline void simple_box_shape(f32 min[3],
				    f32 max[3],
				    f32* verticesOut,
				    u32* indicesOut) {
  f32 vertices[] = {
      //front
      min[0], max[1], min[2],
      max[0], max[1], min[2],
      max[0], min[1], min[2],
      min[0], min[1], min[2],
      //back
      min[0], max[1], max[2],
      max[0], max[1], max[2],
      max[0], min[1], max[2],
      min[0], min[1], max[2],
  };

  u32 indices[] = {
    0, 1, 3,
    3, 1, 2,

    1, 5, 2,
    2, 5, 6,

    5, 4, 6,
    6, 4, 7,

    4, 0, 7,
    7, 0, 3,

    3, 2, 7,
    7, 2, 6,

    4, 5, 0,
    0, 5, 1
  };

  memcpy(verticesOut, vertices, sizeof(vertices));
  memcpy(indicesOut, indices, sizeof(indices));
};

static inline void simple_sphere_shape(f32 radius, i32 stackCount,
                                       i32 sectorCount, f32* verticesOut,
                                       u32* indicesOut) {
  f32 x, y, z, xy;  // vertex position
  //f32 s, t;         // vertex texCoord

  f32 sectorStep = 2.f * PI / sectorCount;
  f32 stackStep = PI / stackCount;
  f32 sectorAngle, stackAngle;
  f32* vIt = verticesOut;
  for (i32 i = 0; i <= stackCount; ++i) {
    stackAngle = PI / 2 - i * stackStep;  // starting from pi/2 to -pi/2
    xy = radius * cosf(stackAngle);       // r * cos(u)
    z = radius * sinf(stackAngle);        // r * sin(u)

    // add (sectorCount+1) vertices per stack
    // first and last vertices have same position and normal, but different tex
    // coords
    for (int j = 0; j <= sectorCount; ++j) {
      sectorAngle = j * sectorStep;  // starting from 0 to 2pi

      // vertex position (x, y, z)
      x = xy * cosf(sectorAngle);  // r * cos(u) * cos(v)
      y = xy * sinf(sectorAngle);  // r * cos(u) * sin(v)
      *vIt = x;
      vIt++;
      *vIt = y;
      vIt++;
      *vIt = z;
      vIt++;

      /* // normalized vertex normal (nx, ny, nz) */
      /* nx = x * lengthInv; */
      /* ny = y * lengthInv; */
      /* nz = z * lengthInv; */
      /* normals.push_back(nx); */
      /* normals.push_back(ny); */
      /* normals.push_back(nz); */

      /* // vertex tex coord (s, t) range between [0, 1] */
      /* s = (float)j / sectorCount; */
      /* t = (float)i / stackCount; */
      /* texCoords.push_back(s); */
      /* texCoords.push_back(t); */
    }
  }

  u32* iIt = indicesOut;
  i32 k1, k2;
  for (i32 i = 0; i < stackCount; ++i) {
    k1 = i * (sectorCount + 1);  // beginning of current stack
    k2 = k1 + sectorCount + 1;   // beginning of next stack

    for (i32 j = 0; j < sectorCount; ++j, ++k1, ++k2) {
      // 2 triangles per sector excluding first and last stacks
      // k1 => k2 => k1+1
      if (i != 0) {
        (*iIt) = k1;
        iIt++;
        (*iIt) = k2;
        iIt++;
        (*iIt) = k1 + 1;
        iIt++;
      }

      // k1+1 => k2 => k2+1
      if (i != (stackCount - 1)) {
        (*iIt) = k1 + 1;
        iIt++;
        (*iIt) = k2;
        iIt++;
        (*iIt) = k2 + 1;
        iIt++;
      }

      /* // store indices for lines */
      /* // vertical lines for all stacks, k1 => k2 */
      /* lineIndices.push_back(k1); */
      /* lineIndices.push_back(k2); */
      /* if (i != 0)  // horizontal lines except 1st stack, k1 => k+1 */
      /* { */
      /*   lineIndices.push_back(k1); */
      /*   lineIndices.push_back(k1 + 1); */
      /* } */
    }
  }
};

static void* (*_malloc)(usize size) = 0;
static void (*_free)(void* ptr) = 0;

static void (*_log)(LogLevel, const char *, ...);
#define ASSERT(cond) _assert(cond, #cond, __FUNCTION__, __LINE__, __FILE__)
static void (*_assert)(b32, const char *, const char *, int, const char *);

/* static void* sokol_malloc(usize size, void* _userData) {return _malloc(size);} */
/* static void sokol_free(void* ptr, void* _userData) {_free(ptr);} */

static void slog_func(const char* tag, u32 log_level, u32 _log_item,
                      const char* message, u32 line_nr,
                      const char* filename, void* _user_data) {
  LogLevel level = LOG_LEVEL_DEBUG;
  switch (log_level) {
    case 0:
    case 1:
      level = LOG_LEVEL_ERROR;
      break;
    case 2:
      level = LOG_LEVEL_WARN;
      break;
    default:
      level = LOG_LEVEL_DEBUG;
      break;
  }
  char logMessage[512];
  snprintf(logMessage, 512, "%s : %s. LineNum: %d FileName: %s",tag, message, line_nr, filename);
  _log(level, logMessage);
}

static inline void setup(renderer_command_setup* cmd) {

  _malloc = cmd->allocFunc;
  _free = cmd->freeFunc;
  _log = cmd->log;
  _assert = cmd->assert;

  sg_desc sgDesc = {0};
  sgDesc.logger.func = slog_func;

  // TODO: still something wonky on my free list implementation
  /* sgDesc.allocator.alloc_fn = sokol_malloc; */
  /* sgDesc.allocator.free_fn = sokol_free; */
  sg_setup(&sgDesc);
  ASSERT(sg_isvalid());
}

static inline void shutdown() {
  sg_shutdown();
}

static inline void free_vertex_buffer(renderer_command_free_vertex_buffer *cmd) {
  sg_destroy_buffer((sg_buffer){cmd->vertexBufferHandle});
  sg_destroy_buffer((sg_buffer){cmd->indexBufferHandle});
}

static inline void free_program_pipeline(renderer_command_free_program_pipeline *cmd) {
  sg_destroy_shader((sg_shader){cmd->shaderProgramHandle});
  sg_destroy_pipeline((sg_pipeline){cmd->pipelineHandle});
}

static inline void free_sampler(renderer_command_free_sampler* cmd) {
  sg_destroy_sampler((sg_sampler){cmd->samplerHandle});
}

static inline void free_image(renderer_command_free_image* cmd) {
  sg_destroy_image((sg_image){cmd->imageHandle});
}

static inline void clear(renderer_command_clear* cmd) {
  sg_pass_action pass_action = {0};
  pass_action.colors[0] = (sg_color_attachment_action){.load_action = SG_LOADACTION_CLEAR,
                           .clear_value =
			   {cmd->clearColor[0],
			    cmd->clearColor[1],
			    cmd->clearColor[2],
			    cmd->clearColor[3]}};

  sg_begin_default_pass(&pass_action, cmd->width, cmd->height);
}

static inline void flip(renderer_command_flip* cmd) {
  sg_end_pass();
  sg_commit();
}

static inline void create_vertex_buffers(renderer_command_create_vertex_buffer* cmd) {
  sg_buffer_desc vBufDesc = {0};
  sg_buffer_desc iBufDesc = {0};

  vBufDesc.type = SG_BUFFERTYPE_VERTEXBUFFER;
  iBufDesc.type = SG_BUFFERTYPE_INDEXBUFFER;

  vBufDesc.usage = cmd->isStreamData ? SG_USAGE_STREAM : SG_USAGE_IMMUTABLE;
  iBufDesc.usage = cmd->isStreamData ? SG_USAGE_STREAM : SG_USAGE_IMMUTABLE;

  vBufDesc.data.size = cmd->vertexDataSize;
  iBufDesc.data.size = cmd->indexDataSize;

  if (cmd->isStreamData == false) {
    iBufDesc.data.ptr = cmd->indexData;
    vBufDesc.data.ptr = cmd->vertexData;
  }
  sg_buffer vbuf = sg_make_buffer(&vBufDesc);
  sg_buffer ibuf = sg_make_buffer(&iBufDesc);

  *cmd->vertexBufHandle = vbuf.id;
  *cmd->indexBufHandle = ibuf.id;
}

static inline void update_vertex_buffer(renderer_command_update_vertex_buffer* cmd) {
  sg_range vRange;
  sg_range iRange;

  vRange.ptr = cmd->vertexData;
  vRange.size = cmd->vertexDataSize;

  iRange.ptr = cmd->indexData;
  iRange.size = cmd->indexDataSize;

  sg_update_buffer((sg_buffer){cmd->vertexBufHandle},&vRange);
  sg_update_buffer((sg_buffer){cmd->indexBufHandle},&iRange);
}

static inline void create_program_pipeline(renderer_command_create_program_pipeline* cmd) {
  sg_shader_desc sDesc = {0};
  sDesc.fs.source = cmd->fragmentShaderData;
  sDesc.vs.source = cmd->vertexShaderData;

  for(u16 i = 0; i < cmd->vertexUniformEntries.num; i++) {
    uniform_entry* uEntry = cmd->vertexUniformEntries.buffer + i;
    sDesc.vs.uniform_blocks[0].uniforms[i] = (sg_shader_uniform_desc){
      .name = uEntry->name,
      .type = (sg_uniform_type)uEntry->type};
  }

  for(u16 i = 0; i < cmd->fragmentUniformEntries.num; i++) {
    uniform_entry* uEntry = cmd->fragmentUniformEntries.buffer + i;
    sDesc.fs.uniform_blocks[0].uniforms[i] = (sg_shader_uniform_desc){
      .name = uEntry->name,
      .type = (sg_uniform_type)uEntry->type};
  }

  sDesc.vs.uniform_blocks[0].size = cmd->vertexUniformEntries.bufferSize;
  sDesc.fs.uniform_blocks[0].size = cmd->fragmentUniformEntries.bufferSize;

  for(u16 i = 0; i < cmd->vertexSamplerEntries.num; i++) {
    sampler_entry* sEntry = cmd->vertexSamplerEntries.buffer + i;
    sDesc.vs.images[i].used = true;
    sDesc.vs.samplers[i].used = true;
    sDesc.vs.image_sampler_pairs[i].used = true;
    sDesc.vs.image_sampler_pairs[i].glsl_name = sEntry->name;
    sDesc.vs.image_sampler_pairs[i].image_slot = i;
    sDesc.vs.image_sampler_pairs[i].sampler_slot = i;
  }

  for(u16 i = 0; i < cmd->fragmentSamplerEntries.num; i++) {
    sampler_entry* sEntry = cmd->fragmentSamplerEntries.buffer + i;
    sDesc.fs.images[i].used = true;
    sDesc.fs.samplers[i].used = true;
    sDesc.fs.image_sampler_pairs[i].used = true;
    sDesc.fs.image_sampler_pairs[i].glsl_name = sEntry->name;
    sDesc.fs.image_sampler_pairs[i].image_slot = i;
    sDesc.fs.image_sampler_pairs[i].sampler_slot = i;
  }


  sg_shader shd = sg_make_shader(&sDesc);

  sg_pipeline pip = {0};
  sg_pipeline_desc pipDesc = {0};

  i32 layoutIdx = 0;
  // Vertex
  pipDesc.layout.attrs[layoutIdx].format = (sg_vertex_format)cmd->layout.position.format;
  pipDesc.layout.attrs[layoutIdx].offset = cmd->layout.position.offset;
  pipDesc.layout.attrs[layoutIdx].buffer_index = cmd->layout.position.layoutIdx;
  // NORMAL
  if (cmd->layout.normals.layoutIdx >= 0) {
    layoutIdx++;
    pipDesc.layout.attrs[layoutIdx].format = (sg_vertex_format)cmd->layout.normals.format;
    pipDesc.layout.attrs[layoutIdx].offset = cmd->layout.normals.offset;
    pipDesc.layout.attrs[layoutIdx].buffer_index = cmd->layout.normals.layoutIdx;
  }
  // UV
  if (cmd->layout.uv.layoutIdx >= 0) {
    layoutIdx++;
    pipDesc.layout.attrs[layoutIdx].format = (sg_vertex_format)cmd->layout.uv.format;
    pipDesc.layout.attrs[layoutIdx].offset = cmd->layout.uv.offset;
    pipDesc.layout.attrs[layoutIdx].buffer_index = cmd->layout.uv.layoutIdx;
  }
  // COLOR
  if (cmd->layout.color.layoutIdx >= 0) {
    layoutIdx++;
    pipDesc.layout.attrs[layoutIdx].format = (sg_vertex_format)cmd->layout.color.format;
    pipDesc.layout.attrs[layoutIdx].offset = cmd->layout.color.offset;
    pipDesc.layout.attrs[layoutIdx].buffer_index = cmd->layout.color.layoutIdx;
  }

  pipDesc.colors[0].blend.enabled = true;
  pipDesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
  pipDesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

  pipDesc.primitive_type = cmd->primitiveType == PRIMITIVE_TRIANGLES ?
    SG_PRIMITIVETYPE_TRIANGLES :
    SG_PRIMITIVETYPE_LINES;

  pipDesc.shader = shd;
  pipDesc.index_type = cmd->u16IndexType ? SG_INDEXTYPE_UINT16 : SG_INDEXTYPE_UINT32;
  if (cmd->depthTest) {
    pipDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pipDesc.depth.write_enabled = true;
  }
  pipDesc.face_winding = cmd->cw ? SG_FACEWINDING_CW : SG_FACEWINDING_CCW;
  pipDesc.cull_mode = SG_CULLMODE_BACK;

  pip = sg_make_pipeline(&pipDesc);

  *cmd->shaderProgramHandle = shd.id;
  *cmd->progPipelineHandle = pip.id;
}

static inline void update_image(renderer_command_update_image* cmd) {
  sg_range range = {0};
  range.ptr = cmd->pixels;
  range.size = cmd->dataSize;
  sg_image_data data = {0};
  data.subimage[0][0] = range;
  sg_update_image((sg_image){cmd->imageHandle}, &data);
};

static inline void create_image(renderer_command_create_image* cmd) {
  sg_pixel_format fmt;
  switch (cmd->components) {
    case 1:
      fmt = (cmd->depth == 8) ? SG_PIXELFORMAT_R8 : SG_PIXELFORMAT_R16;
      break;
    case 2:
      fmt = (cmd->depth == 8) ? SG_PIXELFORMAT_RG8 : SG_PIXELFORMAT_RG16;
      break;
    case 3:
      fmt =
          (cmd->depth == 8) ? SG_PIXELFORMAT_RGBA8 : SG_PIXELFORMAT_RGBA16;
      break;
    case 4:
      fmt =
          (cmd->depth == 8) ? SG_PIXELFORMAT_RGBA8 : SG_PIXELFORMAT_RGBA16;
      break;
    default:
      break;
  }

  sg_image_desc desc = {0};
  desc.width = cmd->width;
  desc.height = cmd->height;

  desc.pixel_format = fmt;
  desc.usage = SG_USAGE_DYNAMIC;
  desc.num_mipmaps = 1;

  u32 imageHandle = sg_make_image(&desc).id;

  if (!imageHandle) {
    printf("Texture creation failed\n");
  }
  renderer_command_update_image imgCmd = (renderer_command_update_image){
      cmd->_header,
      cmd->dataSize,
      cmd->pixels,
      imageHandle};
  update_image(&imgCmd);

  *cmd->imageHandle = imageHandle;
}

static inline void create_sampler(renderer_command_create_sampler *cmd) {
  sg_sampler_desc desc = {0};
  desc.min_filter = SG_FILTER_NEAREST;
  desc.mag_filter = SG_FILTER_NEAREST;
  desc.wrap_u = SG_WRAP_REPEAT;
  desc.wrap_v = SG_WRAP_REPEAT;
  *cmd->samplerHandle = sg_make_sampler(&desc).id;
}

static inline void apply_bindings(renderer_command_apply_bindings *cmd) {

  // fill the resource bindings, note how the same vertex
  // buffer is bound to the first two slots, and the vertex-buffer-offsets
  // are used to poi32 to the position- and color-components.

  sg_bindings bindings ={0};

  u32 layoutIdx = 0;
  u32 bufferOffset = 0;
  bindings.vertex_buffers[layoutIdx].id = cmd->vertexBufferHandle;
  bindings.vertex_buffer_offsets[layoutIdx] = bufferOffset;
  bufferOffset += cmd->bufferLayout.position;
  layoutIdx++;

  if (cmd->bufferLayout.normals > 0) {
    bindings.vertex_buffers[layoutIdx].id = cmd->vertexBufferHandle;
    bindings.vertex_buffer_offsets[layoutIdx] = bufferOffset;
    bufferOffset += cmd->bufferLayout.normals;
    layoutIdx++;
  }

  if (cmd->bufferLayout.uv > 0) {
    bindings.vertex_buffers[layoutIdx].id = cmd->vertexBufferHandle;
    bindings.vertex_buffer_offsets[layoutIdx] = bufferOffset;
    bufferOffset += cmd->bufferLayout.uv;
    layoutIdx++;
  }

  if (cmd->bufferLayout.color > 0) {
    bindings.vertex_buffers[layoutIdx].id = cmd->vertexBufferHandle;
    bindings.vertex_buffer_offsets[layoutIdx] = bufferOffset;
  }

  for (u16 i = 0; i < cmd->vertexSamplerBindings.num; i++) {
    sampler_binding_entry* sd = cmd->vertexSamplerBindings.buffer + i;
    bindings.vs.samplers[i].id = sd->samplerHandle;
    bindings.vs.images[i].id = sd->textureHandle;
  }

  for (u16 i = 0; i < cmd->fragmentSamplerBindings.num; i++) {
    sampler_binding_entry* sd = cmd->fragmentSamplerBindings.buffer + i;
    bindings.fs.samplers[i].id = sd->samplerHandle;
    bindings.fs.images[i].id = sd->textureHandle;
  }

  bindings.index_buffer.id = cmd->indexBufferHandle;
  bindings.index_buffer_offset = cmd->bufferLayout.index;

  sg_apply_bindings(&bindings);

}

static inline void apply_program_pipeline(renderer_command_apply_program_pipeline* cmd) {
  sg_apply_pipeline((sg_pipeline){cmd->pipelineHandle});
}

static inline void apply_uniforms(renderer_command_apply_uniforms* cmd) {
  if (cmd->vertexUniformDataBuffer.bufferSize) {
    sg_range range = {.ptr = cmd->vertexUniformDataBuffer.buffer,
                      .size = cmd->vertexUniformDataBuffer.bufferSize};
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &range);
  }
  if (cmd->fragmentUniformDataBuffer.bufferSize) {
    sg_range range = {.ptr = cmd->fragmentUniformDataBuffer.buffer,
                      .size = cmd->fragmentUniformDataBuffer.bufferSize};
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &range);
  }
}

static inline void draw_elements(renderer_command_draw_elements* cmd) {
  /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */
  sg_draw(cmd->baseElement,
	  cmd->numElement,
	  cmd->numInstance);
}

static inline void simple_draw_prepare_pipeline(sg_pipeline* pip, b32 triangles) {
  if (pip->id == 0) {
    sg_shader shd = sg_make_shader(&simpleShaderDesc);
    sg_pipeline_desc pipelineDesc = {
        .layout.attrs[0] =
            {
	      .format = SG_VERTEXFORMAT_FLOAT3,
	      .buffer_index = 0,
            },
        .colors[0] = {.blend =
                          {
                              .enabled = true,
                              .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                              .dst_factor_rgb =
                                  SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                          }},
        .primitive_type = triangles ? SG_PRIMITIVETYPE_TRIANGLES : SG_PRIMITIVETYPE_LINES,
        .index_type = SG_INDEXTYPE_UINT32,
        .depth.compare = SG_COMPAREFUNC_LESS_EQUAL,
        .depth.write_enabled = true,
        .face_winding = SG_FACEWINDING_CCW,
        .cull_mode = SG_CULLMODE_BACK,
    };

    pipelineDesc.shader = shd;
    *pip = sg_make_pipeline(&pipelineDesc);
  }
  sg_apply_pipeline(*pip);
}

static inline void simple_draw_prepare_buffers(sg_buffer* vbuf, sg_buffer* ibuf,
				       sg_range vRange, sg_range iRange) {
  if (vbuf->id == 0) {
    sg_buffer_desc simpleVertexBufferDesc = {.type = SG_BUFFERTYPE_VERTEXBUFFER,
                                             .usage = SG_USAGE_STREAM,
                                             .data.size = vRange.size};

    sg_buffer_desc simpleIndexBufferDesc = {.type = SG_BUFFERTYPE_INDEXBUFFER,
                                            .usage = SG_USAGE_IMMUTABLE,
                                            .data.ptr = iRange.ptr,
                                            .data.size = iRange.size};

    *vbuf = sg_make_buffer(&simpleVertexBufferDesc);
    *ibuf = sg_make_buffer(&simpleIndexBufferDesc);
  }

  sg_update_buffer(*vbuf, &vRange);
}

static inline void simple_draw_bind(sg_buffer* vbuf, sg_buffer* ibuf,
				    f32* mvp, f32* color) {

  sg_bindings bindings = {.vertex_buffers[0] = *vbuf, .index_buffer = *ibuf};

  sg_apply_bindings(&bindings);

  sg_range vsRange = {.ptr = mvp, .size = sizeof(f32) * 16};
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vsRange);
  sg_range fsRange = {.ptr = color, .size = sizeof(f32) * 4};
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fsRange);
}

static inline void simple_draw_lines(renderer_simple_draw_lines* cmd) {
  f32 *vertices = cmd->lines;
  u32 indices[cmd->lineNum * 2];

  static sg_pipeline pip = {0};
  static sg_buffer vbuf = {0};
  static sg_buffer ibuf = {0};

  u32* it = indices;
  for (u32 i = 0; i < cmd->lineNum * 2; i++) {
    *it = i;
    it++;
  }

  sg_range vRange, iRange;
  vRange.ptr = vertices;
  vRange.size = sizeof(f32) * cmd->lineNum * 6;

  iRange.ptr = indices;
  iRange.size = sizeof(indices);

  simple_draw_prepare_pipeline(&pip, false);
  simple_draw_prepare_buffers(&vbuf, &ibuf, vRange, iRange);
  simple_draw_bind(&vbuf, &ibuf, cmd->mvp, cmd->color);
  sg_draw(0, cmd->lineNum * 6, 1);
  sg_destroy_buffer(vbuf);
  sg_destroy_buffer(ibuf);
  vbuf.id = 0;
  ibuf.id = 0;
}

static inline void simple_draw_box(renderer_simple_draw_box* cmd) {
  f32 vertices[24];
  u32 indices[48];
  simple_box_shape(cmd->min, cmd->max, vertices, indices);
  static sg_pipeline pip = {0};
  static sg_buffer vbuf = {0};
  static sg_buffer ibuf = {0};

  sg_range vRange, iRange;
  vRange.ptr = vertices;
  vRange.size = sizeof(vertices);

  iRange.ptr = indices;
  iRange.size = sizeof(indices);

  simple_draw_prepare_pipeline(&pip, true);
  simple_draw_prepare_buffers(&vbuf, &ibuf, vRange, iRange);
  simple_draw_bind(&vbuf, &ibuf, cmd->mvp, cmd->color);
  sg_draw(0, 36, 1);
  sg_destroy_buffer(vbuf);
  sg_destroy_buffer(ibuf);
  vbuf.id = 0;
  ibuf.id = 0;
}

static inline void simple_draw_sphere(renderer_simple_draw_sphere* cmd) {
  f32 vertices[cmd->sectorCount * cmd->stackCount * 3];
  u32 indices[cmd->sectorCount * cmd->stackCount * 6];

  simple_sphere_shape(cmd->radius, cmd->stackCount, cmd->sectorCount, vertices, indices);

  static sg_pipeline pip = {0};
  static sg_buffer vbuf = {0};
  static sg_buffer ibuf = {0};

  sg_range vRange, iRange;
  vRange.ptr = vertices;
  vRange.size = sizeof(vertices);

  iRange.ptr = indices;
  iRange.size = sizeof(indices);

  simple_draw_prepare_pipeline(&pip, true);
  simple_draw_prepare_buffers(&vbuf, &ibuf, vRange, iRange);
  simple_draw_bind(&vbuf, &ibuf, cmd->mvp, cmd->color);
  sg_draw(0, cmd->sectorCount * cmd->stackCount, 1);
  sg_destroy_buffer(vbuf);
  sg_destroy_buffer(ibuf);
  vbuf.id = 0;
  ibuf.id = 0;
}

extern RT_RENDERER_FLUSH_BUFFER(flushCommandBuffer) {
  for (u32 address = 0; address < buffer->arena.head;) {
    renderer_command_header* header =
      (renderer_command_header*)(buffer->arena.buffer + address);

    if (header->type != RendCmdType_renderer_command_setup) {
      ASSERT(header->type);
    }
    switch (header->type) {
    case RendCmdType_renderer_command_setup: {
      setup((renderer_command_setup*)header);
      address += sizeof(renderer_command_setup);
    }break;
    case RendCmdType_renderer_command_shutdown: {
      shutdown((renderer_command_shutdown*)header);
      address += sizeof(renderer_command_shutdown);
    }break;
    case RendCmdType_renderer_command_free_vertex_buffer: {
      free_vertex_buffer((renderer_command_free_vertex_buffer*)header);
      address += sizeof(renderer_command_free_vertex_buffer);
    }break;
    case RendCmdType_renderer_command_free_program_pipeline: {
      free_program_pipeline((renderer_command_free_program_pipeline*)header);
      address += sizeof(renderer_command_free_program_pipeline);
    }break;
    case RendCmdType_renderer_command_free_sampler: {
      free_sampler((renderer_command_free_sampler*)header);
      address += sizeof(renderer_command_free_sampler);
    }break;
    case RendCmdType_renderer_command_free_image: {
      free_image((renderer_command_free_image*)header);
      address += sizeof(renderer_command_free_image);
    }break;
    case RendCmdType_renderer_command_create_vertex_buffer: {
      create_vertex_buffers((renderer_command_create_vertex_buffer*)header);
      address += sizeof(renderer_command_create_vertex_buffer);
    } break;
    case RendCmdType_renderer_command_create_program_pipeline: {
      create_program_pipeline((renderer_command_create_program_pipeline*)header);
      address += sizeof(renderer_command_create_program_pipeline);
    } break;
    case RendCmdType_renderer_command_create_sampler: {
      create_sampler((renderer_command_create_sampler*)header);
      address += sizeof(renderer_command_create_sampler);
    } break;
    case RendCmdType_renderer_command_create_image: {
      create_image((renderer_command_create_image*)header);
      address += sizeof(renderer_command_create_image);
    } break;
    case RendCmdType_renderer_command_update_vertex_buffer: {
      update_vertex_buffer((renderer_command_update_vertex_buffer*)header);
      address += sizeof(renderer_command_update_vertex_buffer);
    } break;
    case RendCmdType_renderer_command_update_image: {
      update_image((renderer_command_update_image*)header);
      address += sizeof(renderer_command_update_image);
    }break;
    case RendCmdType_renderer_command_apply_bindings: {
      apply_bindings((renderer_command_apply_bindings*)header);
      address += sizeof(renderer_command_apply_bindings);
    } break;
    case RendCmdType_renderer_command_apply_program_pipeline: {
      apply_program_pipeline((renderer_command_apply_program_pipeline*)header);
      address += sizeof(renderer_command_apply_program_pipeline);
    }break;
    case RendCmdType_renderer_command_apply_uniforms: {
      apply_uniforms((renderer_command_apply_uniforms*)header);
      address += sizeof(renderer_command_apply_uniforms);
    }break;
    case RendCmdType_renderer_command_clear: {
      clear((renderer_command_clear*)header);
      address += sizeof(renderer_command_clear);
    }break;
    case RendCmdType_renderer_command_flip: {
      flip((renderer_command_flip*)header);
      address += sizeof(renderer_command_flip);
    } break;
    case RendCmdType_renderer_command_draw_elements: {
      draw_elements((renderer_command_draw_elements*)header);
      address += sizeof(renderer_command_draw_elements);
    } break;
    case RendCmdType_renderer_simple_draw_lines: {
      simple_draw_lines((renderer_simple_draw_lines*)header);
      address += sizeof(renderer_simple_draw_lines);
    } break;
    case RendCmdType_renderer_simple_draw_box: {
      simple_draw_box((renderer_simple_draw_box*)header);
      address += sizeof(renderer_simple_draw_box);
    } break;
    case RendCmdType_renderer_simple_draw_sphere: {
      simple_draw_sphere((renderer_simple_draw_sphere*)header);
      address += sizeof(renderer_simple_draw_sphere);
    } break;
    case RendCmdType_renderer_simple_draw_grid: {} break;
    case RendCmdType_renderer_command_UNDEFINED:
    case _RendCmdType_renderer_command_num:
      InvalidDefaultCase;
    }
  }

  buffer->arena.head = 0;
};
