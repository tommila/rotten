#include <string.h>
#include <stdio.h>

#include <math.h>

#include "glad.h"
#include "types.h"
#include "core.h"
#include "mem.h"
#include "rotten_renderer.h"
#include "glad.c"

#define PI 3.14159265359f

typedef struct simple_draw_data {
  vertex_buffer_handle VBO;
  index_buffer_handle EBO;
  vertex_array_handle VAO;
  shader_program_handle program;
  handle mvpLocation;
  handle colorLocation;
  uptr vertexOffset;
  uptr elementOffset;
} simple_draw_data;

static simple_draw_data sdo = {0};

static const char* simpleShaderVs =
    "#version 330\n"
    "layout(location = 0) in vec3 position;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "  gl_Position = mvp * vec4(position.xyz, 1.f);\n"
    "}\n";

static const char* simpleShaderFs =
    "#version 330\n"
    "out vec4 frag_color;\n"
    "uniform vec4 u_color;\n"
    "void main() {\n"
    "  frag_color = u_color;\n"
    "}\n";

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
    2, 1, 2,

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

static void (*_log)(LogLevel, const char*, ...);
#define ASSERT_MSG(cond, msg, id)                         \
  {                                                       \
    char str[256];                                        \
    snprintf(str, 256, "id %s, msg: %s", id, msg);	  \
    _assert(cond, str, __FUNCTION__, __LINE__, __FILE__); \
  }
#define ASSERT(cond) _assert(cond, #cond, __FUNCTION__, __LINE__, __FILE__)
static void (*_assert)(b32, const char *, const char *, int, const char *);

void begin(rt_render_entry_begin* cmd) {
  sdo.vertexOffset = 0;
  sdo.elementOffset = 0;
}

static inline void shutdown_renderer() {
}

static inline void free_vertex_buffer(rt_render_entry_free_vertex_buffer *cmd) {
  /* sg_destroy_buffer((sg_buffer){cmd->vertexBufferHandle}); */
  /* sg_destroy_buffer((sg_buffer){cmd->indexBufferHandle}); */
}

static inline void free_program_pipeline(rt_render_entry_free_program_pipeline *cmd) {
  /* sg_destroy_shader((sg_shader){cmd->shaderProgramHandle}); */
  /* sg_destroy_pipeline((sg_pipeline){cmd->pipelineHandle}); */
}

static inline void free_sampler(rt_render_entry_free_sampler* cmd) {
  /* sg_destroy_sampler((sg_sampler){cmd->samplerHandle}); */
}

static inline void free_image(rt_render_entry_free_image* cmd) {
  /* sg_destroy_image((sg_image){cmd->imageHandle}); */
}

static inline void clear(rt_render_entry_clear* cmd) {
  glClearColor(cmd->clearColor[0],
			    cmd->clearColor[1],
			    cmd->clearColor[2],
			    cmd->clearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, cmd->width, cmd->height);
}

static inline void flip(rt_render_entry_flip* cmd) {
  /* sg_end_pass(); */
  /* sg_commit(); */
}

static inline void create_vertex_buffers(rt_render_entry_create_vertex_buffer* cmd) {

  u32 VBO, EBO, VAO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER,
	        cmd->vertexDataSize, cmd->vertexData, cmd->isStreamData ? GL_STREAM_DRAW : GL_STATIC_DRAW);

  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::BINDING VERTEX BUFFER %d\n",
         err);
    return;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       cmd->indexDataSize, cmd->indexData, cmd->isStreamData ? GL_STREAM_DRAW : GL_STATIC_DRAW);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::BINDING ELEMENT BUFFER %d\n",
         err);
    return;
  }

  rt_renderer_vertex_attributes *attrib = cmd->vertexAttributes;
  u32 attribIdx = 0;
  while(attrib->count) {
    glVertexAttribPointer(attribIdx, attrib->count, GL_BYTE + attrib->type,
			  attrib->normalized, attrib->stride, (void*)(usize)attrib->offset);
    glEnableVertexAttribArray(attribIdx++);
    attrib++;
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::BINDING VERTEX BUFFER%d\n",
         err);
    return;
  }
  *cmd->vertexArrHandle = VAO;
  *cmd->vertexBufHandle = VBO;
  *cmd->indexBufHandle = EBO;
}

static inline void updateVertexBuffer(rt_render_entry_update_vertex_buffer* cmd) {
  ASSERT_MSG(cmd->vertexBufHandle && cmd->indexBufHandle, "Null vertex or index buffer", cmd->_header.id);
  glBindBuffer(GL_ARRAY_BUFFER, cmd->vertexBufHandle);
  glBufferSubData(GL_ARRAY_BUFFER, cmd->vertexDataOffset,
                  cmd->vertexDataSize, cmd->vertexData);

  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::UPDATE VERTEX BUFFER %d\n",
         err);
    return;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd->indexBufHandle);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, cmd->indexDataOffset,
                  cmd->indexDataSize, cmd->indexData);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::UPDATE ELEMENT BUFFER %d\n",
         err);
    return;
  }
}

static inline void create_shader_shader(const char* vsShaderStr, const char* fsShaderStr,
					u32* vertShader, u32* fragShader) {
  i32 err;
  *vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(*vertShader, 1, &vsShaderStr, NULL);
  glCompileShader(*vertShader);

  i32 success;
  char infoLog[512];
  glGetShaderiv(*vertShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(*vertShader, 512, NULL, infoLog);
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n",infoLog);
    return;
  }
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::ERROR %d\n",err);
    return;
  }
  *fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(*fragShader, 1, &fsShaderStr, NULL);
  glCompileShader(*fragShader);

  glGetShaderiv(*fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(*fragShader, 512, NULL, infoLog);
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED %s\n",infoLog);
    return;
  }
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::ERROR %d\n",err);
    return;
  }
}

static inline void create_shader_program(rt_render_entry_create_shader_program* cmd) {

  i32 err;
  i32 success;
  char infoLog[512];
  u32 vertShader, fragShader, shaderProgram;
  create_shader_shader(cmd->vertexShaderData, cmd->fragmentShaderData, &vertShader, &fragShader);
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::PROGRAM::LINK_FAILED %s\n",infoLog);
    return;
  }

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::ERROR %d\n",err);
    return;
  }
  *cmd->shaderProgramHandle = shaderProgram;

  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
}

static inline void update_shader_program(rt_render_entry_update_shader_program* cmd) {
  ASSERT_MSG(*cmd->shaderProgramHandle,"Null shader program", cmd->_header.id);
  u32 oldProgram = *cmd->shaderProgramHandle;
  create_shader_program((rt_render_entry_create_shader_program*)cmd);
  glDeleteProgram(oldProgram);

  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::PROGRAM_UPDATE%d\n",err);
    return;
  }
}

static inline void update_image(rt_render_entry_update_image* cmd) {
  i32 fmt;
  switch (cmd->components) {
    case 1:
      fmt = (cmd->depth == 8) ? GL_R8 : GL_R16;
      break;
    case 2:
      fmt = (cmd->depth == 8) ? GL_RG8 : GL_RG16;
      break;
    case 3:
      fmt = (cmd->depth == 8) ? GL_RGBA8 : GL_RGBA16;
      break;
    case 4:
      fmt = (cmd->depth == 8) ? GL_RGBA8 : GL_RGBA16;
      break;
    default:
      break;
  }

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cmd->width, cmd->height, fmt, GL_UNSIGNED_BYTE, cmd->pixels);
};

static inline void create_image(rt_render_entry_create_image* cmd) {
  u32 tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  // set the texture wrapping parameters
  // set texture wrapping to GL_REPEAT (default wrapping method)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  i32 fmt;
  switch (cmd->components) {
    case 1:
      fmt = (cmd->depth == 8) ? GL_R8 : GL_R16;
      break;
    case 2:
      fmt = (cmd->depth == 8) ? GL_RG8 : GL_RG16;
      break;
    case 3:
      fmt = (cmd->depth == 8) ? GL_RGBA8 : GL_RGBA16;
      break;
    case 4:
      fmt = (cmd->depth == 8) ? GL_RGBA8 : GL_RGBA16;
      break;
    default:
      break;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, fmt, cmd->width, cmd->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cmd->pixels);

  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::TEXTURE::CREATE %d\n",
         err);
    return;
  }
  *cmd->imageHandle = tex;
}

static inline void create_sampler(rt_render_entry_create_sampler *cmd) {
  /* sg_sampler_desc desc = {0}; */
  /* desc.min_filter = SG_FILTER_NEAREST; */
  /* desc.mag_filter = SG_FILTER_NEAREST; */
  /* desc.wrap_u = SG_WRAP_REPEAT; */
  /* desc.wrap_v = SG_WRAP_REPEAT; */
  /* *cmd->samplerHandle = sg_make_sampler(&desc).id; */
}

static inline void apply_bindings(rt_render_entry_apply_bindings *cmd) {
  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::APPLYING BINDINS %d\n",err);
    return;
  }

  sampler_binding_entry* sd = cmd->textureBindings;
  i32 texIndex = 0;
  while (sd->textureHandle) {
    glActiveTexture(GL_TEXTURE0 + texIndex++); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, sd->textureHandle);
    sd++;
  }

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::APPLYING BINDINS %d\n",err);
    return;
  }

  glBindVertexArray(cmd->vertexArrayHandle);
  glBindBuffer(GL_ARRAY_BUFFER, cmd->vertexBufferHandle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd->indexBufferHandle);
}

static inline void apply_program(rt_render_entry_apply_program* cmd) {
  cmd->enableBlending ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
  if (cmd->enableBlending) {
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  cmd->enableCull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
  if (cmd->enableCull) {
    glCullFace(GL_BACK);
  }
  cmd->enableDepthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
  if (cmd->enableDepthTest) {
    glDepthFunc(GL_LESS);
  }

  glFrontFace(cmd->ccwFrontFace ? GL_CCW : GL_CW);
  glUseProgram(cmd->programHandle);

  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::APPLY_PROGRAM::USE_PROGRAM %d\n",err);
    return;
  }
}

static inline void set_uniform_data(i32 location, uniform_entry* entry) {
  switch (entry->type) {
  case uniform_type_f32:
    glUniform1fv(location, 1, entry->data); break;
  case uniform_type_vec2:
    glUniform2fv(location, 1, entry->data); break;
  case uniform_type_vec3:
    glUniform3fv(location, 1, entry->data); break;
  case uniform_type_vec4:
    glUniform4fv(location, 1, entry->data); break;
  case uniform_type_mat4:
    glUniformMatrix4fv(location, 1, 0, entry->data); break;
  case uniform_type_int:
    glUniform1iv(location, 1, entry->data); break;
  case uniform_type_invalid:
    InvalidDefaultCase;
  }
}

static inline void apply_uniforms(rt_render_entry_apply_uniforms* cmd) {
  uniform_entry* entry = cmd->uniforms;
  ASSERT_MSG(cmd->shaderProgram,"Null shader program", cmd->_header.id);
  while (entry->type) {
    i32 location = glGetUniformLocation(cmd->shaderProgram, entry->name);
    set_uniform_data(location, entry);
    i32 err = glGetError();
    if (err != GL_NO_ERROR) {
      _log(LOG_LEVEL_ERROR, "ERROR::UNIFORMS::APPLYING DATA %d\n",
	   err);
      return;
    }
    entry++;
  }
}

static inline void drawElements(rt_render_entry_draw_elements* cmd) {
  /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */
  glDrawElementsBaseVertex(
		 cmd->mode == rt_primitive_triangles ? GL_TRIANGLES : GL_LINES,
		 cmd->numElement,
		 GL_UNSIGNED_INT,
                 (void*)(cmd->baseElement * sizeof(u32)),
		 cmd->baseVertex);
}

////////////////////////////////
// Simple Draw Implementation //
////////////////////////////////

static inline void updateSimpleDrawBufferData(void* vData, usize vSize,
                                              void* iData, usize iSize) {
  i32 err = glGetError();
  glBindVertexArray(sdo.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, sdo.VBO);
  glBufferSubData(GL_ARRAY_BUFFER, sdo.vertexOffset * 3 * sizeof(f32), vSize, vData);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sdo.EBO);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sdo.elementOffset * sizeof(u32), iSize, iData);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::UPDATE VERTEX BUFFER %d\n",
         err);
  }
}

static inline void applySimpleDrawProgram(f32* mvp, f32* color) {

  glUseProgram(sdo.program);
  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::PROGRAM USE %d\n",err);
    return;
  }
  glUniformMatrix4fv(sdo.mvpLocation, 1, false, mvp);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::UNIFORM BINDING %d\n",err);
    return;
  }
  glUniform4fv(sdo.colorLocation, 1, color);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::UNIFORM BINDING %d\n",err);
    return;
  }
}

static inline void initSimpleDraw() {
  u32 vertShader, fragShader, shaderProgram;
  vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertShader, 1, &simpleShaderVs, NULL);
  glCompileShader(vertShader);
  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SIMPLE::VERTEX::COMPILATION GL ERROR %d\n",
	 err);
    return;
  }
  i32 success;
  char infoLog[512];
  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n",infoLog);
    return;
  }

  fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &simpleShaderFs, NULL);
  glCompileShader(fragShader);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SIMPLE::FRAGMENT::COMPILATION GL ERROR %d\n",
	 err);
    return;
  }
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED %s\n",infoLog);
    return;
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SIMPLE::FRAGMENT::LINK GL ERROR %d\n",
	 err);
    return;
  }
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::PROGRAM::LINK_FAILED %s\n",infoLog);
    return;
  }

  sdo.mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
  sdo.colorLocation = glGetUniformLocation(shaderProgram, "u_color");
  sdo.program = shaderProgram;

  glGenVertexArrays(1, &sdo.VAO);
  glGenBuffers(1, &sdo.VBO);
  glGenBuffers(1, &sdo.EBO);
  glBindVertexArray(sdo.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, sdo.VBO);
  glBufferData(GL_ARRAY_BUFFER,
	        MEGABYTES(1), NULL, GL_STREAM_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sdo.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       MEGABYTES(1), NULL, GL_STREAM_DRAW);


  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::INIT %d\n",err);
    return;
  }

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::BINDING %d\n",err);
    return;
  }
}

static inline void renderSimpleLines(rt_render_simple_lines* cmd) {
  f32 *vertices = cmd->lines;
  u32 indices[cmd->lineNum];

  u32 vertexSize = cmd->lineNum * 3 * sizeof(f32);
  u32 indexSize = sizeof(indices);

  for (u32 i = 0; i < cmd->lineNum; i++) {
    indices[i] = i;
  }

  i32 err = 0;
  applySimpleDrawProgram(cmd->mvp, cmd->color);
  updateSimpleDrawBufferData(vertices, vertexSize,
                              indices, indexSize);
  glLineWidth(cmd->lineWidth ? cmd->lineWidth : 1.f);
  glDrawElementsBaseVertex(GL_LINES, cmd->lineNum, GL_UNSIGNED_INT,
			   (void*)(sdo.elementOffset * sizeof(u32)), sdo.vertexOffset);

  glEnableVertexAttribArray(0);

  sdo.vertexOffset += cmd->lineNum;
  sdo.elementOffset += cmd->lineNum;

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::DRAW ARRAYS %d\n",err);
    return;
  }
}

static inline void renderSimpleBox(rt_render_simple_box* cmd) {
  f32 vertices[24];
  u32 indices[36];
  simple_box_shape(cmd->min, cmd->max, vertices, indices);

  u32 vertexSize = sizeof(vertices);
  u32 indexSize = sizeof(indices);

  i32 err = glGetError();
  err = glGetError();
  applySimpleDrawProgram(cmd->mvp, cmd->color);
  updateSimpleDrawBufferData(vertices, vertexSize,
                             indices, indexSize);
  glDrawElementsBaseVertex(GL_TRIANGLES, arrayLen(indices), GL_UNSIGNED_INT,
			   (void*)(sdo.elementOffset * sizeof(u32)), sdo.vertexOffset);

  glEnableVertexAttribArray(0);

  sdo.vertexOffset += arrayLen(vertices);
  sdo.elementOffset += arrayLen(indices);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::DRAW ARRAYS %d\n",err);
    return;
  }
}

static inline void simple_draw_sphere(rt_render_simple_sphere* cmd) {
  /* f32 vertices[cmd->sectorCount * cmd->stackCount * 3]; */
  /* u32 indices[cmd->sectorCount * cmd->stackCount * 6]; */

  /* simple_sphere_shape(cmd->radius, cmd->stackCount, cmd->sectorCount, vertices, indices); */

  /* static sg_pipeline pip = {0}; */
  /* static sg_buffer vbuf = {0}; */
  /* static sg_buffer ibuf = {0}; */

  /* sg_range vRange, iRange; */
  /* vRange.ptr = vertices; */
  /* vRange.size = sizeof(vertices); */

  /* iRange.ptr = indices; */
  /* iRange.size = sizeof(indices); */

  /* simple_draw_prepare_pipeline(&pip, true); */
  /* updateSimpleDrawBufferData(&vbuf, &ibuf, vRange, iRange); */
  /* applySimpleDrawProgram(&vbuf, &ibuf, cmd->mvp, cmd->color); */
  /* sg_draw(0, cmd->sectorCount * cmd->stackCount, 1); */
  /* sg_destroy_buffer(vbuf); */
  /* sg_destroy_buffer(ibuf); */
  /* vbuf.id = 0; */
  /* ibuf.id = 0; */
}

extern RT_RENDERER_INIT(rendererInit) {
  _log = log;
  _assert = assert;

  if (!gladLoadGLLoader((GLADloadproc) glGetProcAddressFunc)) {
    _log(LOG_LEVEL_ERROR, "ERROR::GLAD LOADER%d\n");
  }

  initSimpleDraw();
}

extern RT_RENDERER_FLUSH_BUFFER(flushCommandBuffer) {
  for (u32 address = 0; address < buffer->arena.head;) {
    rt_render_entry_header* header =
      (rt_render_entry_header*)(buffer->arena.buffer + address);

    if (header->type != RendCmdType_rt_render_entry_begin) {
      ASSERT_MSG(header->type,"Invalid header type", header->id);
    }
    switch (header->type) {
    case RendCmdType_rt_render_entry_begin: {
      begin((rt_render_entry_begin*)header);
      address += sizeof(rt_render_entry_begin);
    }break;
    case RendCmdType_rt_render_entry_shutdown: {
      shutdown_renderer();
      address += sizeof(rt_render_entry_shutdown);
    }break;
    case RendCmdType_rt_render_entry_free_vertex_buffer: {
      free_vertex_buffer((rt_render_entry_free_vertex_buffer*)header);
      address += sizeof(rt_render_entry_free_vertex_buffer);
    }break;
    case RendCmdType_rt_render_entry_free_program_pipeline: {
      free_program_pipeline((rt_render_entry_free_program_pipeline*)header);
      address += sizeof(rt_render_entry_free_program_pipeline);
    }break;
    case RendCmdType_rt_render_entry_free_sampler: {
      free_sampler((rt_render_entry_free_sampler*)header);
      address += sizeof(rt_render_entry_free_sampler);
    }break;
    case RendCmdType_rt_render_entry_free_image: {
      free_image((rt_render_entry_free_image*)header);
      address += sizeof(rt_render_entry_free_image);
    }break;
    case RendCmdType_rt_render_entry_create_vertex_buffer: {
      create_vertex_buffers((rt_render_entry_create_vertex_buffer*)header);
      address += sizeof(rt_render_entry_create_vertex_buffer);
    } break;
    case RendCmdType_rt_render_entry_create_shader_program: {
      create_shader_program((rt_render_entry_create_shader_program*)header);
      address += sizeof(rt_render_entry_create_shader_program);
    } break;
    case RendCmdType_rt_render_entry_create_sampler: {
      create_sampler((rt_render_entry_create_sampler*)header);
      address += sizeof(rt_render_entry_create_sampler);
    } break;
    case RendCmdType_rt_render_entry_create_image: {
      create_image((rt_render_entry_create_image*)header);
      address += sizeof(rt_render_entry_create_image);
    } break;
    case RendCmdType_rt_render_entry_update_vertex_buffer: {
      updateVertexBuffer((rt_render_entry_update_vertex_buffer*)header);
      address += sizeof(rt_render_entry_update_vertex_buffer);
    } break;
    case RendCmdType_rt_render_entry_update_image: {
      update_image((rt_render_entry_update_image*)header);
      address += sizeof(rt_render_entry_update_image);
    }break;
    case RendCmdType_rt_render_entry_update_shader_program: {
      update_shader_program((rt_render_entry_update_shader_program*)header);
      address += sizeof(rt_render_entry_update_shader_program);
    }break;
    case RendCmdType_rt_render_entry_apply_bindings: {
      apply_bindings((rt_render_entry_apply_bindings*)header);
      address += sizeof(rt_render_entry_apply_bindings);
    } break;
    case RendCmdType_rt_render_entry_apply_program: {
      apply_program((rt_render_entry_apply_program*)header);
      address += sizeof(rt_render_entry_apply_program);
    }break;
    case RendCmdType_rt_render_entry_apply_uniforms: {
      apply_uniforms((rt_render_entry_apply_uniforms*)header);
      address += sizeof(rt_render_entry_apply_uniforms);
    }break;
    case RendCmdType_rt_render_entry_clear: {
      clear((rt_render_entry_clear*)header);
      address += sizeof(rt_render_entry_clear);
    }break;
    case RendCmdType_rt_render_entry_flip: {
      flip((rt_render_entry_flip*)header);
      address += sizeof(rt_render_entry_flip);
    } break;
    case RendCmdType_rt_render_entry_draw_elements: {
      drawElements((rt_render_entry_draw_elements*)header);
      address += sizeof(rt_render_entry_draw_elements);
    } break;
    case RendCmdType_rt_render_simple_lines: {
      renderSimpleLines((rt_render_simple_lines*)header);
      address += sizeof(rt_render_simple_lines);
    } break;
    case RendCmdType_rt_render_simple_box: {
      renderSimpleBox((rt_render_simple_box*)header);
      address += sizeof(rt_render_simple_box);
    } break;
    case RendCmdType_rt_render_simple_sphere: {
      simple_draw_sphere((rt_render_simple_sphere*)header);
      address += sizeof(rt_render_simple_sphere);
    } break;
    case RendCmdType_rt_render_simple_grid: {} break;
    case RendCmdType_rt_render_entry_UNDEFINED:
    case _RendCmdType_rt_render_entry_num:
      InvalidDefaultCase;
    }
  }

  buffer->arena.head = 0;
};
