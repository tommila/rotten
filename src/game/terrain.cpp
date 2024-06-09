#include "cglm/struct/vec3.h"
#include "cglm/vec3.h"
#include "car_game.h"

typedef struct terrain_vs_uniform_params {
  mat4s mvp;
  vec2s offset;
  vec2s gridSize;
  vec2s gridCells;
  vec3s heightMapScale;
  vec3s scaleAndFallOf;
} terrain_vs_uniform_params;

static vec2s terrainSizeInMeters = {2048.f, 2048.f};
static vec2s heightMapImgSize = {1024.f, 1024.f};
static vec2s geometrySize = {4096.f, 4096.f};
static vec3s heightMapScale = {geometrySize.x / heightMapImgSize.x,
			       geometrySize.y / heightMapImgSize.y, 80.f};

static f32 terrainMeshGridSizeX = 1024.f;
static f32 terrainMeshGridSizeY = 1024.f;
static f32 terrainMeshCellNumX = 256.f;
static f32 terrainMeshCellNumY = 256.f;

static stbi_uc *getPixelP(stbi_uc *image, u32 imageWidth, u32 imageHeight,
                          ivec2s pos) {
  i32 x = pos.x % imageWidth;
  while (x < 0) x += imageWidth;
  i32 y = pos.y % imageHeight;
  while (y < 0) y += imageHeight;
  return image + (4 * (y * imageWidth + x));
}

static vec4s getPixel(stbi_uc *image, u32 imageWidth, u32 imageHeight,
                      ivec2s pos) {
  stbi_uc *p = getPixelP(image, imageWidth, imageHeight, pos);
  return {(f32)p[0] / 255.f, (f32)p[1] / 255.f, (f32)p[2] / 255.f,
          (f32)p[3] / 255.f};
}

static void setPixel(stbi_uc *image, u32 imageWidth, u32 imageHeight,
                     ivec2s pos, vec4s color) {
  stbi_uc *p = getPixelP(image, imageWidth, imageHeight, pos);
  u8 rgba[4] = {(u8)(color.r * 255.f), (u8)(color.g * 255.f),
                (u8)(color.b * 255.f), (u8)(color.a * 255.f)};
  p[0] = rgba[0];
  p[1] = rgba[1];
  p[2] = rgba[2];
  p[3] = rgba[3];
}

f32 getGeometryHeight(vec3s pos, game_state *game, vec3s *normOut) {
  i32 sizeX = heightMapImgSize.x;
  i32 sizeY = heightMapImgSize.y;

  f32 geomPosX = pos.x / heightMapScale.x + sizeX * 0.5f;
  f32 geomPosY = pos.y / heightMapScale.y + sizeY * 0.5f;

  while (geomPosX < 0) geomPosX += sizeX;
  while (geomPosY < 0) geomPosY += sizeY;

  i32 x = (i32)(geomPosX);
  i32 y = (i32)(geomPosY);
  f32 u = geomPosX - x;
  f32 v = geomPosY - y;

  i32 xPlusOne = (x == sizeX - 1) ? 0 : x + 1;
  i32 yPlusOne = (y == sizeY - 1) ? 0 : y + 1;

  // Approximate heights and normals
  vec4s c00 = game->terrain.geometry[sizeX * y + x];
  vec4s c10 = game->terrain.geometry[sizeX * y + xPlusOne];
  vec4s c01 = game->terrain.geometry[sizeX * yPlusOne + x];
  vec4s c11 = game->terrain.geometry[sizeX * yPlusOne + xPlusOne];

  vec3s n00 = {c00.y,c00.z,c00.w};
  vec3s n10 = {c10.y,c10.z,c10.w};
  vec3s n01 = {c01.y,c01.z,c01.w};
  vec3s n11 = {c11.y,c11.z,c11.w};

  // Bilinear interpolate
  f32 a = c00.x * (1.f - u) + c10.x * u;
  f32 b = c01.x * (1.f - u) + c11.x * u;
  f32 h = a * (1.f - v) + b * v;

  vec3s aN = n00 * (1.f - u) + n10 * u;
  vec3s bN = n01 * (1.f - u) + n11 * u;
  vec3s normal = aN * (1.f - v) + bN * v;

  *normOut = normal;
  return pos.z - h;
}

static void updateGeometryMesh(terrain_object *terrain, vec3s *meshVertices,
                               vec4s *geometry) {
  img_data img = terrain->heightMapImg;
  // NOTE: This assumes that geometry grid vertex count is same
  //       as the heightmap pixel count
  for (i16 y = 0; y < img.height; y++) {
    for (i16 x = 0; x < img.width; x++) {
      vec4s height = getPixel((stbi_uc *)img.pixels, img.width, img.height,
                              (ivec2s){x, y});
      vec4s heightA = getPixel((stbi_uc *)img.pixels, img.width, img.height,
                               (ivec2s){x + 1, y});
      vec4s heightB = getPixel((stbi_uc *)img.pixels, img.width, img.height,
                               (ivec2s){x - 1, y});
      vec4s heightC = getPixel((stbi_uc *)img.pixels, img.width, img.height,
                               (ivec2s){x, y + 1});
      vec4s heightD = getPixel((stbi_uc *)img.pixels, img.width, img.height,
                               (ivec2s){x, y - 1});
      vec3s normal  = glms_vec3_normalize((vec3s){height.y, height.z, height.w} * 2.f - (vec3s){1.f, 1.f, 1.f});
      vec3s normalA = glms_vec3_normalize((vec3s){heightA.y, heightA.z, heightA.w} * 2.f - (vec3s){1.f, 1.f, 1.f});
      vec3s normalB = glms_vec3_normalize((vec3s){heightB.y, heightB.z, heightB.w} * 2.f - (vec3s){1.f, 1.f, 1.f});
      vec3s normalC = glms_vec3_normalize((vec3s){heightC.y, heightC.z, heightC.w} * 2.f - (vec3s){1.f, 1.f, 1.f});
      vec3s normalD = glms_vec3_normalize((vec3s){heightD.y, heightD.z, heightD.w} * 2.f - (vec3s){1.f, 1.f, 1.f});

      vec3s v = meshVertices[(y * img.width) + x];

      // Smooth the geometry and normals a bit
      v.z = (height.x + heightA.x + heightB.x + heightC.x + heightD.x) / 5.f;
      v.z = v.z * heightMapScale.z - heightMapScale.z;
      vec3s n = (normal + normalA + normalB + normalC + normalD) / 5.f;

      meshVertices[(y * img.width) + x] = v;
      geometry[(y * img.width) + x] = (vec4s){v.z, n.x, n.y, n.z};
    }
  }
}

static f32 calcHeight(stbi_uc* pixels, i16 w, i16 h, i16 x, i16 y){
  while (x < 0) x += w;
  while (y < 0) y += h;
  x = x % w;
  y = y % h;
  f32 height = MAX(getPixel(pixels, w, h, (ivec2s){x, y}).x, 0.25f);
  // f32 noise1 = sinf(((f32)y / (f32)w) * PI) * 4.f;
  // height += noise1;

  return height;
}

// Create normal map data from height map and embbed it back to height map
// images g,b,a channels.
static void embbedNormalMapData(img_data img) {
  i16 w = img.width, h = img.height;

  for (i16 x = 0; x < w; x++) {
    for (i16 y = 0; y < h; y++) {
      f32 height = calcHeight((stbi_uc *)img.pixels, w, h, x, y);

      f32 strength = heightMapScale.z;

      f32 l = calcHeight((stbi_uc *)img.pixels, w, h, x - 1, y);
      f32 b = calcHeight((stbi_uc *)img.pixels, w, h, x, y + 1);
      f32 r = calcHeight((stbi_uc *)img.pixels, w, h, x + 1, y);
      f32 t = calcHeight((stbi_uc *)img.pixels, w, h, x, y - 1);

      // Append normals as gba values
      vec3s A  = {0, 0, height * heightMapScale.z};
      vec3s B1 = {heightMapScale.y, 0, r * heightMapScale.z};
      vec3s B2 = {-heightMapScale.y, 0, l * heightMapScale.z};
      vec3s C1 = {0, heightMapScale.y, b * heightMapScale.z};
      vec3s C2 = {0, -heightMapScale.y, t * heightMapScale.z};
      vec3s n = LERP(calcNormal(A, B1, C1), calcNormal(A, B2, C2),0.5);

      // Add normal map values as pixel y,z,w values.
      setPixel((stbi_uc *)img.pixels, w, h, (ivec2s){x, y},
               {height, (n.x + 1.f) * 0.5f, (n.y + 1.f) * 0.5f,
                (n.z + 1.f) * 0.5f});
    }
  }
}

static void deleteTerrain(renderer_command_buffer *rendererBuffer,
                          game_state *game) {
  terrain_object *terrain = &game->terrain;
  // Terrain mesh
  {
    renderer_command_free_image *cmd = renderer_pushCommandArray(
        rendererBuffer, renderer_command_free_image, 2);
    cmd->imageHandle = terrain->terrain_model.terrainTexHandle;
    cmd++;
    cmd->imageHandle = terrain->terrain_model.heightMapTexHandle;

    terrain->terrain_model.terrainTexHandle = 0;
    terrain->terrain_model.heightMapTexHandle = 0;
  }
  {
    renderer_command_free_sampler *cmd = renderer_pushCommandArray(
        rendererBuffer, renderer_command_free_sampler, 2);
    cmd->samplerHandle = terrain->terrain_model.terrainSamplerHandle;
    cmd++;
    cmd->samplerHandle = terrain->terrain_model.heightMapSamplerHandle;

    terrain->terrain_model.terrainSamplerHandle = 0;
    terrain->terrain_model.heightMapSamplerHandle = 0;
  }
  {
    renderer_command_free_vertex_buffer *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_vertex_buffer);
    cmd->vertexBufferHandle = terrain->terrain_model.vertexBufferHandle;
    cmd->indexBufferHandle = terrain->terrain_model.indexBufferHandle;

    terrain->terrain_model.vertexBufferHandle = 0;
    terrain->terrain_model.indexBufferHandle = 0;
  }
  {
    renderer_command_free_program_pipeline *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_program_pipeline);
    cmd->pipelineHandle = terrain->terrain_model.pipelineHandle;
    cmd->shaderProgramHandle = terrain->terrain_model.programHandle;

    terrain->terrain_model.pipelineHandle = 0;
    terrain->terrain_model.programHandle = 0;
  }

  // Terrain geometry
  {
    renderer_command_free_vertex_buffer *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_vertex_buffer);
    cmd->vertexBufferHandle = terrain->geometry_model.vertexBufferHandle;
    cmd->indexBufferHandle = terrain->geometry_model.indexBufferHandle;

    terrain->geometry_model.vertexBufferHandle = 0;
    terrain->geometry_model.indexBufferHandle = 0;
  }
  {
    renderer_command_free_program_pipeline *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_program_pipeline);
    cmd->pipelineHandle = terrain->geometry_model.pipelineHandle;
    cmd->shaderProgramHandle = terrain->geometry_model.programHandle;

    terrain->geometry_model.pipelineHandle = 0;
    terrain->geometry_model.programHandle = 0;
  }
}

static void createTerrain(memory_arena *permanentArena, memory_arena *tempArena,
                          renderer_command_buffer *rendererBuffer,
                          game_state *state) {
  terrain_object *terrain = &state->terrain;
  img_data terrainImgData = importImage(tempArena, "assets/sand_base.png", 4);
  img_data heightMapImg = importImage(permanentArena, "assets/cell_noise.png", 4);

  // Terrain images and samplers
  {
    renderer_command_create_image *cmd = renderer_pushCommandArray(
        rendererBuffer, renderer_command_create_image, 2);

    cmd->width = terrainImgData.width;
    cmd->height = terrainImgData.height;
    cmd->depth = terrainImgData.depth;
    cmd->components = terrainImgData.components;
    cmd->dataSize = terrainImgData.dataSize;
    cmd->pixels = terrainImgData.pixels;
    cmd->imageHandle = &terrain->terrain_model.terrainTexHandle;
    cmd++;

    cmd->width = heightMapImg.width;
    cmd->height = heightMapImg.height;
    cmd->depth = heightMapImg.depth;
    cmd->components = heightMapImg.components;
    cmd->dataSize = heightMapImg.dataSize;
    cmd->pixels = heightMapImg.pixels;
    cmd->imageHandle = &terrain->terrain_model.heightMapTexHandle;
    terrain->heightMapImg = heightMapImg;

    embbedNormalMapData(heightMapImg);
  }
  {
    renderer_command_create_sampler *cmd = renderer_pushCommandArray(
        rendererBuffer, renderer_command_create_sampler, 2);
    cmd->samplerHandle = &terrain->terrain_model.terrainSamplerHandle;
    cmd++;
    cmd->samplerHandle = &terrain->terrain_model.heightMapSamplerHandle;
  }
  // Terrain mesh
  {
    mesh_data terrainMeshData = mesh_GridShape(
      tempArena,
      terrainMeshGridSizeX,
      terrainMeshGridSizeY,
      terrainMeshCellNumX,
      terrainMeshCellNumY,
      false, false, PRIMITIVE_TRIANGLES);

    renderer_command_create_vertex_buffer *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_create_vertex_buffer);
    cmd->vertexData = terrainMeshData.vertexData;
    cmd->indexData = terrainMeshData.indices;
    cmd->vertexDataSize = sizeof(f32) * (terrainMeshData.vertexNum +
					terrainMeshData.normalNum +
					terrainMeshData.texCoordNum +
					terrainMeshData.texCoordNum);
    cmd->indexDataSize = sizeof(u32) * terrainMeshData.indexNum;
    cmd->isStreamData = false;
    cmd->vertexBufHandle = &terrain->terrain_model.vertexBufferHandle;
    cmd->indexBufHandle = &terrain->terrain_model.indexBufferHandle;

    terrain->terrain_model.elementNum = terrainMeshData.indexNum;
    terrain->terrain_model.vertexBufferLayout = {
      terrainMeshData.vertexNum * (u32)sizeof(f32),
      terrainMeshData.normalNum * (u32)sizeof(f32),
      terrainMeshData.texCoordNum * (u32)sizeof(f32),
      terrainMeshData.colorNum * (u32)sizeof(f32)};
  }
  // Terrain pipelines and shaders
  {
    shader_data terrainShader =
        importShader(tempArena, "./assets/shaders/terrain.vs",
                     "./assets/shaders/terrain.fs");

    renderer_command_create_program_pipeline *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_create_program_pipeline);

    cmd->fragmentShaderData = (char *)terrainShader.fsData;
    cmd->vertexShaderData = (char *)terrainShader.vsData;
    cmd->vertexUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 8);
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, mat4,
                                 "mvp");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, vec2,
                                 "offset");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, vec2,
                                 "gridSize");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, vec2,
                                 "gridCells");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, vec3,
                                 "heightMapScale");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, vec3,
                                 "scaleAndFallOf");

    cmd->vertexSamplerEntries = rt_renderer_allocSamplerBuffer(tempArena, 4);
    cmd->fragmentSamplerEntries = rt_renderer_allocSamplerBuffer(tempArena, 4);
    rt_renderer_pushSamplerEntry(&cmd->vertexSamplerEntries, "height_map");
    rt_renderer_pushSamplerEntry(&cmd->fragmentSamplerEntries, "terrain_tex");

    cmd->layout = {.position = {.format = vertex_format_float3, .layoutIdx = 0},
                   .normals = {.layoutIdx = -1},
                   .uv = {.layoutIdx = -1},
                   .color = {.layoutIdx = -1}};

    cmd->depthTest = true;
    cmd->shaderProgramHandle = &terrain->terrain_model.programHandle;
    cmd->progPipelineHandle = &terrain->terrain_model.pipelineHandle;

    terrain->terrain_model.vsUniformSize = cmd->vertexUniformEntries.bufferSize;
  }
  // Terrain geometry mesh
  {
    terrain->geometry = pushArray(permanentArena,
				  heightMapImg.width * heightMapImg.height, vec4s);
    mesh_data geometryMeshData =
        mesh_GridShape(tempArena,
                       geometrySize.x,
                       geometrySize.y,
                       heightMapImg.width,
		       heightMapImg.height, false,
                       false, PRIMITIVE_LINES);
    updateGeometryMesh(terrain,
		       (vec3s *)geometryMeshData.vertexData,
                       terrain->geometry);
    terrain->geometry_model.elementNum = geometryMeshData.indexNum;
    terrain->geometry_model.vertexBufferLayout = {
        geometryMeshData.vertexNum * (u32)sizeof(f32),
	geometryMeshData.normalNum * (u32)sizeof(f32),
        geometryMeshData.texCoordNum * (u32)sizeof(f32),
        geometryMeshData.colorNum * (u32)sizeof(f32)};

    {
      renderer_command_create_vertex_buffer *cmd = renderer_pushCommand(
          rendererBuffer, renderer_command_create_vertex_buffer);
      cmd->vertexData = geometryMeshData.vertexData;
      cmd->indexData = geometryMeshData.indices;
      cmd->vertexDataSize = sizeof(f32) * (geometryMeshData.vertexNum +
					   geometryMeshData.normalNum +
                                           geometryMeshData.texCoordNum +
                                           geometryMeshData.colorNum);
      cmd->indexDataSize = sizeof(u32) * geometryMeshData.indexNum;
      cmd->isStreamData = false;
      cmd->vertexBufHandle = &terrain->geometry_model.vertexBufferHandle;
      cmd->indexBufHandle = &terrain->geometry_model.indexBufferHandle;
    }
  }
  // Terrain geometry shaders and piplines
  {
    shader_data shader =
        importShader(tempArena, "./assets/shaders/mesh_color.vs",
                     "./assets/shaders/mesh_color.fs");
    {
      renderer_command_create_program_pipeline *cmd = renderer_pushCommand(
          rendererBuffer, renderer_command_create_program_pipeline);

      cmd->fragmentShaderData = (char *)shader.fsData;
      cmd->vertexShaderData = (char *)shader.vsData;
      cmd->vertexUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 8);
      cmd->fragmentUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 8);
      rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, mat4, "mvp");
      rt_renderer_pushUniformEntry(&cmd->fragmentUniformEntries, vec4, "u_color");

      cmd->layout = {
          .position = {.format = vertex_format_float3, .layoutIdx = 0},
	  .normals = {.layoutIdx = -1},
          .uv = {.layoutIdx = -1},
          .color = {.layoutIdx = -1}};
      cmd->primitiveType = PRIMITIVE_LINES;
      cmd->depthTest = true;
      cmd->shaderProgramHandle = &terrain->geometry_model.programHandle;
      cmd->progPipelineHandle = &terrain->geometry_model.pipelineHandle;
    }
  }
}

static void drawTerrain(memory_arena *tempArena, game_state *state,
                        renderer_command_buffer *rendererBuffer,
			mat4s view, mat4s proj, vec3s carPosition) {
  mat4s vpMat = proj * view;

  if (isBitSet(state->deve.drawState, DRAW_TERRAIN_GEOMETRY_NORMALS)) {
    u32 lineNum = 10;
    u32 lineNumTotal = lineNum * lineNum;
    f32 lineSpace = 2.f;
    vec3s *lines = pushArray(tempArena, lineNumTotal * 2, vec3s);
    vec4s *colors = pushArray(tempArena, lineNumTotal * 2, vec4s);

    u32 idx = 0;
    for (u32 x = 0.f; x < lineNum; x++) {
      for (u32 y = 0.f; y < lineNum; y++) {
        vec3s pos = {(f32)carPosition.x + x * lineSpace -
                         (f32)lineNum * lineSpace * 0.5f,
                     (f32)carPosition.y + y * lineSpace -
                         (f32)lineNum * lineSpace * 0.5f,
                     0};

        vec3s normal;
        f32 depth = getGeometryHeight(pos, state, &normal);
        lines[idx] = {pos.x, pos.y, -depth};
        lines[idx + 1] = (vec3s){pos.x, pos.y, -depth} +
                         (vec3s){normal.x, normal.y, normal.z};
        colors[idx] = normalToF32RGBA(normal);
        colors[idx + 1] = normalToF32RGBA(normal);
        idx += 2;
      }
    }
    mat4s vpMat = proj * view;
    renderer_simple_draw_lines *cmd =
        renderer_pushCommand(rendererBuffer, renderer_simple_draw_lines);
    cmd->lines = (f32 *)lines->raw;
    cmd->lineNum = lineNumTotal;
    vec4s c = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    copyType(&c, cmd->color, vec4s);
    copyType(vpMat.raw, cmd->mvp, mat4s);
  }
  if (isBitSet(state->deve.drawState, DRAW_TERRAIN_GEOMETRY)) {
    {
      {
        renderer_command_apply_program_pipeline *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_program_pipeline);
        cmd->pipelineHandle = state->terrain.geometry_model.pipelineHandle;
      }
      {
        renderer_command_apply_bindings *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_bindings);
        cmd->indexBufferHandle =
            state->terrain.geometry_model.indexBufferHandle;
        cmd->vertexBufferHandle =
            state->terrain.geometry_model.vertexBufferHandle;
        cmd->bufferLayout = state->terrain.geometry_model.vertexBufferLayout;
      }
      {
        renderer_command_apply_uniforms *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_uniforms);
	cmd->vertexUniformDataBuffer = rt_renderer_allocUniformDataBuffer(tempArena, sizeof(mat4s));
	mat4s *vsUniform = rt_renderer_pushUniformDataEntry(&cmd->vertexUniformDataBuffer, mat4s);
        *vsUniform = vpMat;

	cmd->fragmentUniformDataBuffer = rt_renderer_allocUniformDataBuffer(tempArena, sizeof(vec4s));
	vec4s *fsUniform = rt_renderer_pushUniformDataEntry(&cmd->fragmentUniformDataBuffer, vec4s);
        *fsUniform = {0.0f, 0.4f, 0.0f, 1.0f};
      }
      {
        renderer_command_draw_elements *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_draw_elements);
        cmd->baseElement = 0;
        cmd->numElement = state->terrain.geometry_model.elementNum;
        cmd->numInstance = 1;
      }
    }
  }
  if (isBitSet(state->deve.drawState, DRAW_TERRAIN)) {
    {
      {
        renderer_command_apply_program_pipeline *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_program_pipeline);
        cmd->pipelineHandle = state->terrain.terrain_model.pipelineHandle;
      }
      {
        renderer_command_apply_bindings *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_bindings);
        cmd->indexBufferHandle = state->terrain.terrain_model.indexBufferHandle;
        cmd->vertexBufferHandle =
            state->terrain.terrain_model.vertexBufferHandle;
        cmd->bufferLayout = state->terrain.terrain_model.vertexBufferLayout;
	cmd->vertexSamplerBindings = rt_renderer_allocSamplerBindingBuffer(tempArena, 4);
	cmd->fragmentSamplerBindings = rt_renderer_allocSamplerBindingBuffer(tempArena, 4);
        rt_renderer_pushSamplerBindingEntry(
            &cmd->vertexSamplerBindings,
            state->terrain.terrain_model.heightMapSamplerHandle,
            state->terrain.terrain_model.heightMapTexHandle);
        rt_renderer_pushSamplerBindingEntry(
            &cmd->fragmentSamplerBindings,
            state->terrain.terrain_model.terrainSamplerHandle,
            state->terrain.terrain_model.terrainTexHandle);
      }
      // Setup and draw outer terrain mesh
      {
        renderer_command_apply_uniforms *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_uniforms);

        cmd->vertexUniformDataBuffer = rt_renderer_allocUniformDataBuffer(
            tempArena, sizeof(terrain_vs_uniform_params));
        terrain_vs_uniform_params *vsParams =
            (terrain_vs_uniform_params *)cmd->vertexUniformDataBuffer.buffer;

        vsParams->mvp = proj * view;
        vsParams->offset = {state->car.body.chassis.position.x,
                            state->car.body.chassis.position.y};
        vsParams->gridSize = {terrainMeshGridSizeX, terrainMeshGridSizeY};
        vsParams->gridCells = {terrainMeshCellNumX, terrainMeshCellNumY};

        vsParams->heightMapScale = heightMapScale;
        vsParams->scaleAndFallOf = {5.0f, 1.0f, 0.9f};
	cmd->vertexUniformDataBuffer.bufferSize = state->terrain.terrain_model.vsUniformSize;
      }
      {
        renderer_command_draw_elements *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_draw_elements);
        cmd->baseElement = 0;
        cmd->numElement = state->terrain.terrain_model.elementNum;
        cmd->numInstance = 1;
      }
      // Setup and draw inner terrain mesh
      {
        renderer_command_apply_uniforms *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_uniforms);

        cmd->vertexUniformDataBuffer = rt_renderer_allocUniformDataBuffer(
            tempArena, sizeof(terrain_vs_uniform_params));
        terrain_vs_uniform_params *vsParams =
            (terrain_vs_uniform_params *)cmd->vertexUniformDataBuffer.buffer;

        vsParams->mvp = proj * view;
        vsParams->offset = {state->car.body.chassis.position.x,
                            state->car.body.chassis.position.y};
        vsParams->gridSize = {terrainMeshGridSizeX, terrainMeshGridSizeY};
        vsParams->gridCells = {terrainMeshCellNumX, terrainMeshCellNumY};

        vsParams->heightMapScale = heightMapScale;
        vsParams->scaleAndFallOf = {1.0f, 0.5f, 1.0f};

	cmd->vertexUniformDataBuffer.bufferSize = state->terrain.terrain_model.vsUniformSize;
      }
      {
        renderer_command_draw_elements *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_draw_elements);
        cmd->baseElement = 0;
        cmd->numElement = state->terrain.terrain_model.elementNum;
        cmd->numInstance = 1;
      }
    }
  }
}
