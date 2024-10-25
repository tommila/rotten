#include <string.h>
#include <stdio.h>

#include "types.h"
#include "core.h"
#include "mem.h"
#include "math.h"
#include "string.h"
#include "glad.h"
#include "glad.c"
#include "rotten_renderer.h"

typedef struct simple_draw_data {
  rt_vertex_buffer_handle VBO;
  rt_index_buffer_handle EBO;
  rt_vertex_array_handle VAO;
  rt_shader_program_handle program;
  rt_handle projViewLocation;
  rt_handle modelLocation;
  rt_handle colorLocation;
  uptr vertexOffset;
  uptr elementOffset;
} simple_draw_data;

static simple_draw_data sdo = {0};

static const char* simpleShaderVs =
    "#version 330\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec3 normal0;\n"
    "uniform mat4 proj_view_mat;\n"
    "uniform mat4 model_mat;\n"
    "out vec3 normal;\n"
    "void main() {\n"
    "  gl_Position = proj_view_mat * model_mat * vec4(position.xyz, 1.0f);\n"
    "  mat3 norm_matrix = transpose(inverse(mat3(model_mat)));\n"
    "  normal = normalize(norm_matrix * normal0);\n"
    "}\n";

static const char* simpleShaderFs =
    "#version 330\n"
    "in vec3 normal;\n"
    "out vec4 frag_color;\n"
    "uniform vec4 u_color;\n"
    "const vec3 light_dir = vec3(0.0f, 0.4f, 0.6f);\n"
    "void main() {\n"
    "  vec3 l = normalize(light_dir);\n"
    "  float d = max(dot(normal, l), 0.4f);\n"
    "  frag_color = vec4(u_color.rgb * d,u_color.a);\n"
    "}\n";

static inline void simple_box_shape(v3 min,
				    v3 max,
				    f32* verticesOut,
				    u32* indicesOut) {
  f32 vertices[] = { // Front face
    // Top face 
    min.x, min.y, max.z,  0.0f,  0.0f,  1.0f, // Bottom-left
    max.x, min.y, max.z,  0.0f,  0.0f,  1.0f, // Bottom-right
    max.x, max.y, max.z,  0.0f,  0.0f,  1.0f, // Top-right
    min.x, max.y, max.z,  0.0f,  0.0f,  1.0f, // Top-left
    
    // Bottom face    
    min.x, min.y, min.z,  0.0f,  0.0f, -1.0f, // Bottom-left
    max.x, min.y, min.z,  0.0f,  0.0f, -1.0f, // Bottom-right
    max.x, max.y, min.z,  0.0f,  0.0f, -1.0f, // Top-right
    min.x, max.y, min.z,  0.0f,  0.0f, -1.0f, // Top-left

    // Left face
    min.x, min.y, min.z, -1.0f,  0.0f,  0.0f, // Bottom-left
    min.x, min.y, max.z, -1.0f,  0.0f,  0.0f, // Bottom-right
    min.x, max.y, max.z, -1.0f,  0.0f,  0.0f, // Top-right
    min.x, max.y, min.z, -1.0f,  0.0f,  0.0f, // Top-left

    // Right face
    max.x, min.y, max.z,  1.0f,  0.0f,  0.0f, // Bottom-right
    max.x, min.y, min.z,  1.0f,  0.0f,  0.0f, // Bottom-left
    max.x, max.y, min.z,  1.0f,  0.0f,  0.0f, // Top-left
    max.x, max.y, max.z,  1.0f,  0.0f,  0.0f, // Top-right

    // Front face
    min.x, max.y, max.z,  0.0f,  1.0f,  0.0f, // Bottom-left
    max.x, max.y, max.z,  0.0f,  1.0f,  0.0f, // Bottom-right
    max.x, max.y, min.z,  0.0f,  1.0f,  0.0f, // Top-right
    min.x, max.y, min.z,  0.0f,  1.0f,  0.0f, // Top-left

    // Back face
    max.x, min.y, max.z,  0.0f, -1.0f,  0.0f, // Bottom-right
    min.x, min.y, max.z,  0.0f, -1.0f,  0.0f, // Bottom-left
    min.x, min.y, min.z,  0.0f, -1.0f,  0.0f, // Top-left,
    max.x, min.y, min.z,  0.0f, -1.0f,  0.0f  // Top-right
  };

  u32 indices[] = {
    // Front face
    0,  1,  2,
    2,  3,  0,

    // Back face
    4,  5,  6,
    6,  7,  4,

    // Left face
    8,  9, 10,
    10, 11,  8,

    // Right face
    12, 13, 14,
    14, 15, 12,

    // Top face
    16, 17, 18,
    18, 19, 16,

    // Bottom face
    20, 21, 22,
    22, 23, 20
  };

  memcpy(verticesOut, vertices, sizeof(vertices));
  memcpy(indicesOut, indices, sizeof(indices));
};

static inline void simple_arrow_shape(f32 length,
				    f32 size,
				    f32* verticesOut,
				    u32* indicesOut) {
  f32 halfSize = size * 0.5f;

  f32 vertices[] = {
    // Arrow Shaft
    // Top face 
    0,      -halfSize, halfSize,  0.0f,  0.0f,  1.0f, // Bottom-left
    length, -halfSize, halfSize,  0.0f,  0.0f,  1.0f, // Bottom-right
    length,  halfSize, halfSize,  0.0f,  0.0f,  1.0f, // Top-right
    0,       halfSize, halfSize,  0.0f,  0.0f,  1.0f, // Top-left
     
    // Bottom face    
    0,       -halfSize, -halfSize,  0.0f,  0.0f, -1.0f, // Bottom-left
    length,  -halfSize, -halfSize,  0.0f,  0.0f, -1.0f, // Bottom-right
    length,   halfSize, -halfSize,  0.0f,  0.0f, -1.0f, // Top-right
    0,        halfSize, -halfSize,  0.0f,  0.0f, -1.0f, // Top-left
     
    // Left face
    0, -halfSize, -halfSize, -1.0f,  0.0f,  0.0f, // Bottom-left
    0, -halfSize,  halfSize, -1.0f,  0.0f,  0.0f, // Bottom-right
    0,  halfSize,  halfSize, -1.0f,  0.0f,  0.0f, // Top-right
    0,  halfSize, -halfSize, -1.0f,  0.0f,  0.0f, // Top-left
     
    // Right face
    length, -halfSize,  halfSize,  1.0f,  0.0f,  0.0f, // Bottom-right
    length, -halfSize, -halfSize,  1.0f,  0.0f,  0.0f, // Bottom-left
    length,  halfSize, -halfSize,  1.0f,  0.0f,  0.0f, // Top-left
    length,  halfSize,  halfSize,  1.0f,  0.0f,  0.0f, // Top-right
    
    // Front face
    0,      halfSize,  halfSize,  0.0f,  1.0f,  0.0f, // Bottom-left
    length, halfSize,  halfSize,  0.0f,  1.0f,  0.0f, // Bottom-right
    length, halfSize, -halfSize,  0.0f,  1.0f,  0.0f, // Top-right
    0,      halfSize, -halfSize,  0.0f,  1.0f,  0.0f, // Top-left
   
    // Back face
    length, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f, // Bottom-right
    0,      -halfSize,  halfSize,  0.0f, -1.0f,  0.0f, // Bottom-left
    0,      -halfSize, -halfSize,  0.0f, -1.0f,  0.0f, // Top-left,
    length, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f, // Top-right
     
    //Arrow Point 
    // Top face   
    length,       size, size, 0.0f,  0.0f,  1.0f, // Bottom-left
    length * 2.f, 0.0f, 0.0f, 0.0f,  0.0f,  1.0f, // Bottom-right
    length ,     -size, size, 0.0f,  0.0f,  1.0f, // Top-right
     
    // Bottom face    
    length,        size, -size, 0.0f,  0.0f, -1.0f, // Bottom-left
    length * 2.f,  0.0f,  0.0f, 0.0f,  0.0f, -1.0f, // Bottom-right
    length,       -size, -size, 0.0f,  0.0f, -1.0f, // Top-right
      
    // Front face
    length,        size,  size, 0.0f,  -1.0f, 0.0f, // Bottom-left
    length * 2.f,  0.0f,  0.0f, 0.0f,  -1.0f, 0.0f, // Bottom-right
    length,        size, -size, 0.0f,  -1.0f, 0.0f, // Top-right
   
    // Back face 
    length,        -size,  size, 0.0f, 1.0f,  0.0f, // Bottom-right
    length * 2.f,  0.0f,   0.0f, 0.0f, 1.0f,  0.0f, // Bottom-left
    length,        -size, -size, 0.0f, 1.0f,  0.0f, // Top-left,

    // Back face 
    length, -size, -size, 1.0f,  0.0f,  0.0f, // Bottom-left
    length, -size,  size, 1.0f,  0.0f,  0.0f, // Bottom-right
    length,  size,  size, 1.0f,  0.0f,  0.0f, // Top-right
    length,  size, -size, 1.0f,  0.0f,  0.0f  // Top-left
  };

  u32 indices[] = {
   // Arrow shaft
   // Front face
    0,  1,  2,
    2,  3,  0,

    // Back face
    4,  5,  6,
    6,  7,  4,

    // Left face
    8,  9, 10,
    10, 11,  8,

    // Right face
    12, 13, 14,
    14, 15, 12,

    // Top face
    16, 17, 18,
    18, 19, 16,

    // Bottom face
    20, 21, 22,
    22, 23, 20,

    // Arrow point
    // Side faces
    24, 25, 26,
    27, 28, 29,
    30, 31, 32,
    33, 34, 35,
    
    36, 37, 38,
    38, 39, 36
    // Back face
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

void begin(rt_command_begin* cmd) {
  sdo.vertexOffset = 0;
  sdo.elementOffset = 0;
}

static inline void shutdownRenderer() {
}

static inline void freeVertexBuffer(rt_command_free_vertex_buffer *cmd) {
}

static inline void freeProgramPipeline(rt_command_free_program_pipeline *cmd) {
}

static inline void freeSample(rt_command_free_sampler* cmd) {
}

static inline void freeTexture(rt_command_free_texture* cmd) {
}

static inline void clear(rt_command_clear* cmd) {
  glClearColor(cmd->clearColor[0],
			    cmd->clearColor[1],
			    cmd->clearColor[2],
			    cmd->clearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, cmd->width, cmd->height);
}

static inline void flip(rt_command_flip* cmd) {
}

static inline void createVertexBuffer(rt_command_create_vertex_buffer* cmd) {

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

  rt_vertex_attributes *attrib = cmd->vertexAttributes;
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

static inline void updateVertexBuffer(rt_command_update_vertex_buffer* cmd) {
  ASSERT_MSG(cmd->vertexBufHandle && cmd->indexBufHandle, "Null vertex or index buffer", 
             TO_C(cmd->_header.id));
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

static inline void create_shader_shader(str8 vsShaderStr, str8 fsShaderStr,
					u32* vertShader, u32* fragShader) {
  i32 err;
  *vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(*vertShader, 1,(const char**)&vsShaderStr.buffer, NULL);
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
  glShaderSource(*fragShader, 1, (const char**)&fsShaderStr.buffer, NULL);
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

static inline void createShaderProgram(rt_command_create_shader_program* cmd) {

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

static inline void updateShaderProgram(rt_command_update_shader_program* cmd) {
  ASSERT_MSG(*cmd->shaderProgramHandle,"Null shader program", TO_C(cmd->_header.id));
  u32 oldProgram = *cmd->shaderProgramHandle;
  createShaderProgram((rt_command_create_shader_program*)cmd);
  glDeleteProgram(oldProgram);

  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::PROGRAM_UPDATE%d\n",err);
    return;
  }
}

static inline void updateTexture(rt_command_update_texture* cmd) {
  i32 fmt = 0;
  i32 texNum = cmd->textureType == rt_texture_type_2d ? 1 : 6;
  for (i32 i = 0; i < texNum; i++) {
    switch (cmd->image[i].components) {
      case 1:
        fmt =  GL_RED;
        break;
      case 2:
        fmt =  GL_RG;
        break;
      case 3:
        fmt =  GL_RGB;
        break;
      case 4:
        fmt =  GL_RGBA;
        break;
        InvalidDefaultCase;
    }

    glTexSubImage2D(cmd->textureType == rt_texture_type_2d ?
                    GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, 0, 0, cmd->image[i].width, cmd->image[i].height,
                    fmt, GL_UNSIGNED_BYTE, cmd->image[i].pixels);
  }
};

static inline void createTexture(rt_command_create_texture* cmd) {
  u32 tex;
  i32 fmt = 0;
  u32 texMode = cmd->textureType == rt_texture_type_2d ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
  i32 texNum = cmd->textureType == rt_texture_type_2d ? 1 : 6;
  glGenTextures(1, &tex);
  glBindTexture(texMode, tex);
  for (i32 i = 0; i < texNum; i++) {
    switch (cmd->image[i].components) {
      case 1:
        fmt =  GL_RED;
        break;
      case 2:
        fmt =  GL_RG;
        break;
      case 3:
        fmt =  GL_RGB;
        break;
      case 4:
        fmt =  GL_RGBA;
        break;
        InvalidDefaultCase;
    }

    glTexImage2D(cmd->textureType == rt_texture_type_2d ?
                GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, fmt, cmd->image[i].width, cmd->image[i].height,
                0, fmt, GL_UNSIGNED_BYTE, cmd->image[i].pixels);
  }

  // set the texture wrapping parameters
  // set texture wrapping to GL_REPEAT (default wrapping method)
  glTexParameteri(texMode, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(texMode, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(texMode, GL_TEXTURE_WRAP_R, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(texMode, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(texMode, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::TEXTURE::CREATE %d\n",
         err);
    return;
  }
  *cmd->imageHandle = tex;
}

static inline void createSampler(rt_command_create_sampler *cmd) {
  /* sg_sampler_desc desc = {0}; */
  /* desc.min_filter = SG_FILTER_NEAREST; */
  /* desc.mag_filter = SG_FILTER_NEAREST; */
  /* desc.wrap_u = SG_WRAP_REPEAT; */
  /* desc.wrap_v = SG_WRAP_REPEAT; */
  /* *cmd->samplerHandle = sg_make_sampler(&desc).id; */
}

static inline void applyBindings(rt_command_apply_bindings *cmd) {
  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::APPLYING BINDINS %d\n",err);
    return;
  }

  rt_binding_data* sd = cmd->textureBindings;
  i32 texIndex = 0;
  while (sd->textureHandle) {
    glActiveTexture(GL_TEXTURE0 + texIndex++); // activate the texture unit first before binding texture
    glBindTexture(sd->textureType == rt_texture_type_2d ?
                  GL_TEXTURE_2D :
                  GL_TEXTURE_CUBE_MAP, sd->textureHandle);
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

static inline void applyPipeline(rt_command_apply_program* cmd) {
  cmd->enableBlending ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
  if (cmd->enableBlending) {
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  cmd->enableCull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
  if (cmd->enableCull) {
    glCullFace(GL_BACK);
  }
  cmd->enableScissorTest ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
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

static inline void set_uniform_data(i32 location, rt_uniform_data* entry) {
  switch (entry->type) {
  case rt_uniform_type_f32:
    glUniform1fv(location, 1, entry->data); break;
  case rt_uniform_type_vec2:
    glUniform2fv(location, 1, entry->data); break;
  case rt_uniform_type_vec3:
    glUniform3fv(location, 1, entry->data); break;
  case rt_uniform_type_vec4:
    glUniform4fv(location, 1, entry->data); break;
  case rt_uniform_type_mat4:
    glUniformMatrix4fv(location, 1, 0, entry->data); break;
  case rt_uniform_type_int:
    glUniform1iv(location, 1, entry->data); break;
  case rt_uniform_type_invalid:
    InvalidDefaultCase;
  }
}

static inline void applyUniforms(rt_command_apply_uniforms* cmd) {
  rt_uniform_data* entry = cmd->uniforms;
  ASSERT_MSG(cmd->shaderProgram,"Null shader program", TO_C(cmd->_header.id));
  while (entry->type) {
    i32 location = glGetUniformLocation(cmd->shaderProgram, TO_C(entry->name));
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

static inline void drawElements(rt_command_draw_elements* cmd) {
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glScissor(cmd->scissor.x,cmd->scissor.y, cmd->scissor.z, cmd->scissor.w);
  glLineWidth(cmd->lineWidth ? cmd->lineWidth : 1.f);
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
  glBufferSubData(GL_ARRAY_BUFFER, sdo.vertexOffset * 6 * sizeof(f32), vSize, vData);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sdo.EBO);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sdo.elementOffset * sizeof(u32), iSize, iData);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::UPDATE VERTEX BUFFER %d\n",
         err);
  }
}

static inline void applySimpleDrawProgram(m4x4 projView, m4x4 model, v4 color) {

  glUseProgram(sdo.program);
  i32 err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::PROGRAM USE %d\n",err);
    return;
  }
  glUniformMatrix4fv(sdo.projViewLocation, 1, false, projView.arr);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::UNIFORM BINDING %d\n",err);
    return;
  }
  glUniformMatrix4fv(sdo.modelLocation, 1, false, model.arr);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::SHADER::UNIFORM BINDING %d\n",err);
    return;
  }
  glUniform4fv(sdo.colorLocation, 1, color.arr);
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

  sdo.projViewLocation = glGetUniformLocation(shaderProgram, "proj_view_mat");
  sdo.modelLocation = glGetUniformLocation(shaderProgram, "model_mat");
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

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(f32)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    _log(LOG_LEVEL_ERROR, "ERROR::VERTEX BUFFER::BINDING %d\n",err);
    return;
  }
}

static inline void renderSimpleLines(rt_command_render_simple_lines* cmd) {
  v3 vertices[cmd->lineNum * 2];
  u32 indices[cmd->lineNum];

  u32 vertexSize = sizeof(vertices);
  u32 indexSize = sizeof(indices);

  for (u32 i = 0; i < cmd->lineNum; i++) {
    indices[i] = i;
    // position
    vertices[i * 2] = cmd->lines[i];
    // normals
    vertices[i * 2 + 1] = (v3){0.0f,0.0,1.f};
  }
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  i32 err = 0;
  applySimpleDrawProgram(cmd->projView, cmd->model, cmd->color);
  updateSimpleDrawBufferData(vertices, vertexSize,
                              indices, indexSize);
  glLineWidth(cmd->lineWidth ? cmd->lineWidth : 1.f);
  glDrawElementsBaseVertex(GL_LINES, arrayLen(indices), GL_UNSIGNED_INT,
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

static inline void renderSimpleBox(rt_command_render_simple_box* cmd) {
  f32 vertices[24 * 6]; // vertices and normals
  u32 indices[36];
  simple_box_shape(cmd->min, cmd->max, vertices, indices);

  u32 vertexSize = sizeof(vertices);
  u32 indexSize = sizeof(indices);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  i32 err = glGetError();
  err = glGetError();
  applySimpleDrawProgram(cmd->projView, cmd->model, cmd->color);
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

static inline void renderSimpleArrow(rt_command_render_simple_arrow* cmd) {
  f32 vertices[(40) * 6];
  u32 indices[54];
  simple_arrow_shape(cmd->length, cmd->size, vertices, indices);

  u32 vertexSize = sizeof(vertices);
  u32 indexSize = sizeof(indices);

  i32 err = glGetError();
  err = glGetError();
  applySimpleDrawProgram(cmd->projView, cmd->model, cmd->color);
  updateSimpleDrawBufferData(vertices, vertexSize,
                             indices, indexSize);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
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
    rt_command_header* header =
      (rt_command_header*)(buffer->arena.buffer + address);

    if (header->type != rt_command_type_begin) {
      ASSERT_MSG(header->type,"Invalid header type", TO_C(header->id));
    }
    switch (header->type) {
    case rt_command_type_begin: {
      begin((rt_command_begin*)header);
      address += sizeof(rt_command_begin);
    }break;
    case rt_command_type_shutdown: {
      shutdownRenderer();
      address += sizeof(rt_command_shutdown);
    }break;
    case rt_command_type_free_vertex_buffer: {
      freeVertexBuffer((rt_command_free_vertex_buffer*)header);
      address += sizeof(rt_command_free_vertex_buffer);
    }break;
    case rt_command_type_free_program_pipeline: {
      freeProgramPipeline((rt_command_free_program_pipeline*)header);
      address += sizeof(rt_command_free_program_pipeline);
    }break;
    case rt_command_type_free_sampler: {
      freeSample((rt_command_free_sampler*)header);
      address += sizeof(rt_command_free_sampler);
    }break;
    case rt_command_type_free_texture: {
      freeTexture((rt_command_free_texture*)header);
      address += sizeof(rt_command_free_texture);
    }break;
    case rt_command_type_create_vertex_buffer: {
      createVertexBuffer((rt_command_create_vertex_buffer*)header);
      address += sizeof(rt_command_create_vertex_buffer);
    } break;
    case rt_command_type_create_shader_program: {
      createShaderProgram((rt_command_create_shader_program*)header);
      address += sizeof(rt_command_create_shader_program);
    } break;
    case rt_command_type_create_sampler: {
      createSampler((rt_command_create_sampler*)header);
      address += sizeof(rt_command_create_sampler);
    } break;
    case rt_command_type_create_texture: {
      createTexture((rt_command_create_texture*)header);
      address += sizeof(rt_command_create_texture);
    } break;
    case rt_command_type_update_vertex_buffer: {
      updateVertexBuffer((rt_command_update_vertex_buffer*)header);
      address += sizeof(rt_command_update_vertex_buffer);
    } break;
    case rt_command_type_update_texture: {
      updateTexture((rt_command_update_texture*)header);
      address += sizeof(rt_command_update_texture);
    }break;
    case rt_command_type_update_shader_program: {
      updateShaderProgram((rt_command_update_shader_program*)header);
      address += sizeof(rt_command_update_shader_program);
    }break;
    case rt_command_type_apply_bindings: {
      applyBindings((rt_command_apply_bindings*)header);
      address += sizeof(rt_command_apply_bindings);
    } break;
    case rt_command_type_apply_program: {
      applyPipeline((rt_command_apply_program*)header);
      address += sizeof(rt_command_apply_program);
    }break;
    case rt_command_type_apply_uniforms: {
      applyUniforms((rt_command_apply_uniforms*)header);
      address += sizeof(rt_command_apply_uniforms);
    }break;
    case rt_command_type_clear: {
      clear((rt_command_clear*)header);
      address += sizeof(rt_command_clear);
    }break;
    case rt_command_type_flip: {
      flip((rt_command_flip*)header);
      address += sizeof(rt_command_flip);
    } break;
    case rt_command_type_draw_elements: {
      drawElements((rt_command_draw_elements*)header);
      address += sizeof(rt_command_draw_elements);
    } break;
    case rt_command_type_render_simple_lines: {
      renderSimpleLines((rt_command_render_simple_lines*)header);
      address += sizeof(rt_command_render_simple_lines);
    } break;
    case rt_command_type_render_simple_box: {
      renderSimpleBox((rt_command_render_simple_box*)header);
      address += sizeof(rt_command_render_simple_box);
    } break;
    case rt_command_type_render_simple_arrow: {
      renderSimpleArrow((rt_command_render_simple_arrow*)header);
      address += sizeof(rt_command_render_simple_arrow);
    } break;
    case rt_command_type_render_simple_sphere: {} break;
    case rt_command_type_render_simple_grid: {} break;
    case rt_command_type_UNDEFINED:
    case _rt_command_type_num:
      InvalidDefaultCase;
    }
  }

  buffer->arena.head = 0;
};
