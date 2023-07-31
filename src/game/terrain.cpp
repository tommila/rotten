#include "all.h"

typedef struct terrain_vs_uniform_params {
  mat4s mvp;
  vec2s offset;
  vec2s gridSize;
  vec2s gridCells;
  vec3s heightMapScale;
  vec3s scaleAndFallOf;
  i32 heightMapSamplerId;
  i32 terrainTexSamplerId;
} terrain_vs_uniform_params;

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
  f32 height = getPixel(pixels, w, h, (ivec2s){x, y}).x;
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

static void terrainCreateModel(game_state *state,
			       memory_arena *permanentArena, memory_arena *tempArena,
			       rt_render_entry_buffer *rendererBuffer,
			       utime assetModTime) {
  terrain_object *terrain = &state->terrain;

  utime terrainTexModTime =
      platformApi->diskIOReadModTime("assets/sand_base.png");
  utime heightMapTexModTime = platformApi->diskIOReadModTime("assets/cell_noise.png");

  // Terrain images and samplers
  if (assetModTime < terrainTexModTime || assetModTime < heightMapTexModTime) {
    img_data terrainImgData = importImage(tempArena, "assets/sand_base.png", 4);
    img_data heightMapImg = importImage(permanentArena, "assets/cell_noise.png", 4);

    if (terrain->terrain_model.terrainTexHandle == 0) {
      rt_render_entry_create_image *cmd = rt_renderer_pushEntryArray(
          rendererBuffer, rt_render_entry_create_image, 2);

      cmd->width = heightMapImg.width;
      cmd->height = heightMapImg.height;
      cmd->depth = heightMapImg.depth;
      cmd->components = heightMapImg.components;
      cmd->pixels = heightMapImg.pixels;
      cmd->imageHandle = &terrain->terrain_model.heightMapTexHandle;
      terrain->heightMapImg = heightMapImg;

      cmd++;
      cmd->width = terrainImgData.width;
      cmd->height = terrainImgData.height;
      cmd->depth = terrainImgData.depth;
      cmd->components = terrainImgData.components;
      cmd->pixels = terrainImgData.pixels;
      cmd->imageHandle = &terrain->terrain_model.terrainTexHandle;

      embbedNormalMapData(heightMapImg);
    }

    // Terrain mesh
    {
      mesh_data terrainMeshData =
          mesh_GridShape(tempArena, terrainMeshGridSizeX, terrainMeshGridSizeY,
                         terrainMeshCellNumX, terrainMeshCellNumY, false, false,
                         rt_primitive_triangles);

      rt_render_entry_create_vertex_buffer *cmd = rt_renderer_pushEntry(
          rendererBuffer, rt_render_entry_create_vertex_buffer);
      cmd->vertexData = terrainMeshData.vertexData;
      cmd->indexData = terrainMeshData.indices;
      cmd->vertexDataSize = terrainMeshData.vertexDataSize;
      cmd->indexDataSize = terrainMeshData.indexDataSize;
      cmd->isStreamData = false;
      cmd->vertexArrHandle = &terrain->terrain_model.vertexArrayHandle;
      cmd->vertexBufHandle = &terrain->terrain_model.vertexBufferHandle;
      cmd->indexBufHandle = &terrain->terrain_model.indexBufferHandle;

      cmd->vertexAttributes[0] = {.count = 3,
                                  .offset = 0,
                                  .stride = 3 * sizeof(f32),
                                  .type = rt_renderer_data_type_f32};

      terrain->terrain_model.elementNum = terrainMeshData.indexNum;
    }
    // Terrain geometry mesh
    {
      terrain->geometry = pushArray(
          permanentArena, heightMapImg.width * heightMapImg.height, vec4s);
      mesh_data geometryMeshData = mesh_GridShape(
          tempArena, geometrySize.x, geometrySize.y, heightMapImg.width,
          heightMapImg.height, false, false, rt_primitive_lines);
      updateGeometryMesh(terrain, (vec3s *)geometryMeshData.vertexData,
                         terrain->geometry);
      terrain->geometry_model.elementNum = geometryMeshData.indexNum;
      {
        rt_render_entry_create_vertex_buffer *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_create_vertex_buffer);
        cmd->vertexData = geometryMeshData.vertexData;
        cmd->indexData = geometryMeshData.indices;
        cmd->vertexDataSize = geometryMeshData.vertexDataSize;
        cmd->indexDataSize = geometryMeshData.indexDataSize;
        cmd->isStreamData = false;
        cmd->vertexBufHandle = &terrain->geometry_model.vertexBufferHandle;
        cmd->indexBufHandle = &terrain->geometry_model.indexBufferHandle;
        cmd->vertexArrHandle = &terrain->geometry_model.vertexArrayHandle;

        cmd->vertexAttributes[0] = {
            .count = 3, .offset = 0, .stride = 3 * sizeof(f32),
	    .type = rt_renderer_data_type_f32};
      }
    }
  }
  // Terrain pipelines and shaders
  utime vsTerrainShaderModTime =
    platformApi->diskIOReadModTime("./assets/shaders/terrain.vs");
  utime fsTerrainShaderModTime =
    platformApi->diskIOReadModTime("./assets/shaders/terrain.fs");
  if (assetModTime < vsTerrainShaderModTime || assetModTime < fsTerrainShaderModTime) {
    shader_data terrainShader =
        importShader(tempArena, "./assets/shaders/terrain.vs",
                     "./assets/shaders/terrain.fs");
    if (terrain->terrain_model.programHandle == 0) {
      rt_render_entry_create_shader_program *cmd = rt_renderer_pushEntry(
          rendererBuffer, rt_render_entry_create_shader_program);

      cmd->fragmentShaderData = (char *)terrainShader.fsData;
      cmd->vertexShaderData = (char *)terrainShader.vsData;
      cmd->shaderProgramHandle = &terrain->terrain_model.programHandle;
    }
  }
  // // Terrain geometry shaders and piplines
  utime vsGeomShaderModTime =
    platformApi->diskIOReadModTime("./assets/shaders/mesh_color.vs");
  utime fsGeomShaderModTime =
    platformApi->diskIOReadModTime("./assets/shaders/mesh_color.fs");
  if (assetModTime < vsGeomShaderModTime || assetModTime < fsGeomShaderModTime) {
    shader_data shader =
        importShader(tempArena, "./assets/shaders/mesh_color.vs",
                     "./assets/shaders/mesh_color.fs");
    if (terrain->geometry_model.programHandle == 0) {
      rt_render_entry_create_shader_program *cmd = rt_renderer_pushEntry(
          rendererBuffer, rt_render_entry_create_shader_program);

      cmd->fragmentShaderData = (char *)shader.fsData;
      cmd->vertexShaderData = (char *)shader.vsData;
      cmd->shaderProgramHandle = &terrain->geometry_model.programHandle;
    }
  }
}

static void terrainRender(game_state *state, memory_arena *tempArena,
			  rt_render_entry_buffer *rendererBuffer,
			  mat4s view, mat4s proj) {
  if (isBitSet(state->deve.drawState, DRAW_TERRAIN)) {
    {
      {
        rt_render_entry_apply_bindings *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_apply_bindings);
        cmd->indexBufferHandle = state->terrain.terrain_model.indexBufferHandle;
        cmd->vertexBufferHandle =
            state->terrain.terrain_model.vertexBufferHandle;
        cmd->vertexArrayHandle = state->terrain.terrain_model.vertexArrayHandle;

        cmd->textureBindings[0] = (sampler_binding_entry){
            .textureHandle = state->terrain.terrain_model.heightMapTexHandle,
            .samplerHandle =
                state->terrain.terrain_model.heightMapSamplerHandle};
        cmd->textureBindings[1] = (sampler_binding_entry){
            .textureHandle = state->terrain.terrain_model.terrainTexHandle,
            .samplerHandle = state->terrain.terrain_model.terrainSamplerHandle};
      }
      {
        rt_render_entry_apply_program *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_apply_program);
        cmd->programHandle = state->terrain.terrain_model.programHandle;
        cmd->ccwFrontFace = true;
        cmd->enableBlending = true;
        cmd->enableCull = true;
        cmd->enableDepthTest = true;
      }
      // // Setup and draw outer terrain mesh
      {
        rt_render_entry_apply_uniforms *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_apply_uniforms);
        cmd->shaderProgram = state->terrain.terrain_model.programHandle;
        terrain_vs_uniform_params *vsParams =
            pushType(tempArena, terrain_vs_uniform_params);

        vsParams->mvp = proj * view;
        vsParams->offset = {state->car.body.chassis.position.x,
                            state->car.body.chassis.position.y};
        vsParams->gridSize = {terrainMeshGridSizeX, terrainMeshGridSizeY};
        vsParams->gridCells = {terrainMeshCellNumX, terrainMeshCellNumY};

        vsParams->heightMapScale = heightMapScale;
        vsParams->scaleAndFallOf = {5.0f, 1.0f, 0.9f};

        cmd->uniforms[0] = (uniform_entry){
            .type = uniform_type_mat4, .name = "mvp", .data = &vsParams->mvp};
        cmd->uniforms[1] = (uniform_entry){.type = uniform_type_vec2,
                                           .name = "offset",
                                           .data = &vsParams->offset};
        cmd->uniforms[2] = (uniform_entry){.type = uniform_type_vec2,
                                           .name = "gridSize",
                                           .data = &vsParams->gridSize};
        cmd->uniforms[3] = (uniform_entry){.type = uniform_type_vec2,
                                           .name = "gridCells",
                                           .data = &vsParams->gridCells};
        cmd->uniforms[4] = (uniform_entry){.type = uniform_type_vec3,
                                           .name = "heightMapScale",
                                           .data = &vsParams->heightMapScale};
        cmd->uniforms[5] = (uniform_entry){.type = uniform_type_vec3,
                                           .name = "scaleAndFallOf",
                                           .data = &vsParams->scaleAndFallOf};
      }
      {
        rt_render_entry_draw_elements *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_draw_elements);
        cmd->numElement = state->terrain.terrain_model.elementNum;
      }
      // Setup and draw inner terrain mesh
      {
        rt_render_entry_apply_uniforms *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_apply_uniforms);

        terrain_vs_uniform_params *vsParams =
            pushType(tempArena, terrain_vs_uniform_params);
        cmd->shaderProgram = state->terrain.terrain_model.programHandle;

        vsParams->mvp = proj * view;
        vsParams->offset = {state->car.body.chassis.position.x,
                            state->car.body.chassis.position.y};
        vsParams->gridSize = {terrainMeshGridSizeX, terrainMeshGridSizeY};
        vsParams->gridCells = {terrainMeshCellNumX, terrainMeshCellNumY};

        vsParams->heightMapScale = heightMapScale;
        vsParams->scaleAndFallOf = {1.0f, 0.5f, 1.0f};

        vsParams->heightMapSamplerId = 0;
        vsParams->terrainTexSamplerId = 1;

        cmd->uniforms[0] = (uniform_entry){
            .type = uniform_type_mat4, .name = "mvp", .data = &vsParams->mvp};
        cmd->uniforms[1] = (uniform_entry){.type = uniform_type_vec2,
                                           .name = "offset",
                                           .data = &vsParams->offset};
        cmd->uniforms[2] = (uniform_entry){.type = uniform_type_vec2,
                                           .name = "gridSize",
                                           .data = &vsParams->gridSize};
        cmd->uniforms[3] = (uniform_entry){.type = uniform_type_vec2,
                                           .name = "gridCells",
                                           .data = &vsParams->gridCells};
        cmd->uniforms[4] = (uniform_entry){.type = uniform_type_vec3,
                                           .name = "heightMapScale",
                                           .data = &vsParams->heightMapScale};
        cmd->uniforms[5] = (uniform_entry){.type = uniform_type_vec3,
                                           .name = "scaleAndFallOf",
                                           .data = &vsParams->scaleAndFallOf};
        cmd->uniforms[6] =
            (uniform_entry){.type = uniform_type_int,
                            .name = "height_map",
                            .data = &vsParams->heightMapSamplerId};
        cmd->uniforms[7] =
            (uniform_entry){.type = uniform_type_int,
                            .name = "terrain_tex",
                            .data = &vsParams->terrainTexSamplerId};
      }
      {
        rt_render_entry_draw_elements *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_draw_elements);
        cmd->numElement = state->terrain.terrain_model.elementNum;
      }
    }
  }
  if (isBitSet(state->deve.drawState, DRAW_TERRAIN_GEOMETRY)) {
    {
      rt_render_entry_apply_program *cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_program);
      cmd->programHandle = state->terrain.geometry_model.programHandle;
      cmd->ccwFrontFace = true;
      cmd->enableBlending = true;
      cmd->enableCull = false;
      cmd->enableDepthTest = true;
    }
    {
      rt_render_entry_apply_bindings *cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_bindings);
      cmd->indexBufferHandle = state->terrain.geometry_model.indexBufferHandle;
      cmd->vertexBufferHandle = state->terrain.geometry_model.vertexBufferHandle;
      cmd->vertexArrayHandle = state->terrain.geometry_model.vertexArrayHandle;
    }
    {
      rt_render_entry_apply_uniforms *cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_uniforms);
      uptr uniformData =
          (uptr)pushSize(tempArena, sizeof(mat4s) + sizeof(vec4s));
      mat4s *mvp = (mat4s *)uniformData;
      *mvp = proj * view;
      uniformData += sizeof(mat4s);
      vec4s *color = (vec4s *)uniformData;
      *color = {0.0f, 0.4f, 0.0f, 1.0f};

      cmd->uniforms[0] = (uniform_entry){
          .type = uniform_type_mat4, .name = "mvp", .data = mvp};
      cmd->uniforms[1] = (uniform_entry){
          .type = uniform_type_vec4, .name = "color", .data = color};
      cmd->shaderProgram = state->terrain.geometry_model.programHandle;
    }
    {
      rt_render_entry_draw_elements *cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_draw_elements);
      cmd->mode = rt_primitive_lines;
      cmd->numElement = state->terrain.geometry_model.elementNum;
    }
  }
  if (isBitSet(state->deve.drawState, DRAW_TERRAIN_GEOMETRY_NORMALS)) {
    u32 lineNum = 10;
    u32 lineNumTotal = lineNum * lineNum * 2;
    f32 lineSpace = 2.f;
    vec3s *lines = pushArray(tempArena, lineNumTotal, vec3s);

    u32 idx = 0;
    vec3s carPosition = state->car.body.chassis.position;
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
        idx += 2;
      }
    }
    mat4s vpMat = proj * view;
    rt_render_simple_lines *cmd =
        rt_renderer_pushEntry(rendererBuffer, rt_render_simple_lines);
    cmd->lines = (f32 *)lines->raw;
    cmd->lineNum = lineNumTotal;
    vec4s c = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    copyType(&c, cmd->color, vec4s);
    copyType(vpMat.raw, cmd->mvp, mat4s);
  }

  // Test stuff

  // if(1){
  //   rt_render_simple_lines *cmd =
  //     rt_renderer_pushEntry(rendererBuffer, rt_render_simple_lines);

  //   u32 lineNum = 30;
  //   u32 lineNumTotal = lineNum * 2;
  //   u32 lineIdx = 0;
  //   // static vec3s lines[60];
  //   vec3s *lines = pushArray(tempArena, lineNumTotal, vec3s);
  //   for (u32 i = 0; i < lineNum; i++) {
  //     f32 t1 = (f32)i / (f32)lineNum;
  //     f32 t2 = (f32)(i + 1) / (f32)lineNum;
  //     f32 x1 = -0.8 + t1 * 1.6f;
  //     f32 x2 = -0.8 + t2 * 1.6f;
  //     f32 b1 = bezier5n(0.0f, 4.f, -5.f, 2.f, -0.f, 0.0f, t1);
  //     f32 b2 = bezier5n(0.0f, 4.f, -5.f, 2.f, -0.f, 0.0f, t2);

  //     lines[lineIdx] = (vec3s){x1, b1, 0.0f};
  //     lines[lineIdx + 1] = (vec3s){x2, b2, 0.0f};
  //     lineIdx += 2;
  //   }

  //   cmd->lines = (f32 *)(lines);
  //   cmd->lineNum = lineNumTotal;
  //   cmd->lineWidth = 2.f;
  //   mat4s m = GLMS_MAT4_IDENTITY_INIT;
  //   vec4s c = (vec4s){1.0f, 0.0f, 1.0f, 1.0f};
  //   copyType(&c, cmd->color, vec4s);
  //   copyType(m.raw, cmd->mvp, mat4s);
  //   platformApi->renderer_flushCommandBuffer(rendererBuffer);
  // }
  // if(1){
  //   u32 boxNum = 500;
  //   rt_render_simple_box *cmd =
  //     rt_renderer_pushEntryArray(rendererBuffer, rt_render_simple_box, boxNum);

  //   u32 xSplit = 20;
  //   for (u32 i = 0; i < boxNum; i++) {
  //     f32 x = -1.0f + (f32)(i % xSplit) / xSplit * 2.f;
  //     f32 y = -1.0f + (f32)(i / xSplit) / (f32)xSplit * 2.f;
  //     shape_box box =
  //         createBoxShape((vec3s){-0.01f + x, -0.01f + y, -0.01f},
  //                        (vec3s){0.01f + x, 0.01f + y, 0.01f});
  //     mat4s m = GLMS_MAT4_IDENTITY_INIT;
  //     // mat4s m = glms_translate_make({x, y, 0.0f});
  //     vec4s c = (vec4s){1.0f, 0.0f, 1.0f, 1.0f};
  //     copyType(box.min.raw, cmd->min, vec3s);
  //     copyType(box.max.raw, cmd->max, vec3s);
  //     copyType(&c, cmd->color, vec4s);
  //     copyType(m.raw, cmd->mvp, mat4s);

  //     cmd++;
  //   }
  // }
}

static void terrainInit(game_state *game,
			memory_arena *permanentArena,
			memory_arena *tempArena,
			rt_render_entry_buffer *rendererBuffer,
			b32 reload,
			f32 assetModTime) {
  if (reload || !game->terrain.initialized) {
    terrainCreateModel(game, permanentArena, tempArena, rendererBuffer, assetModTime);
    game->terrain.initialized = true;
  }
}
