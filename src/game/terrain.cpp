#include "all.h"

typedef struct terrain_vs_uniform_params {
  m4x4 mvp;
  v3 offset;
  v2 gridSize;
  v2 gridCells;
  v3 heightMapScale;
  v3 scaleAndFallOf;
  i32 heightMapSamplerId;
  i32 terrainTexSamplerId;
} terrain_vs_uniform_params;

static v2 heightMapImgSize = {1024.f, 1024.f};
static v2 geometrySize = {4096.f, 4096.f};
static v3 heightMapScale = {geometrySize.x / heightMapImgSize.x,
                            geometrySize.y / heightMapImgSize.y, 80.f};

static f32 terrainMeshGridSizeX = 1024.f;
static f32 terrainMeshGridSizeY = 1024.f;
static f32 terrainMeshCellNumX = 256.f;
static f32 terrainMeshCellNumY = 256.f;

static u8 *getPixelP(u8 *image, u32 imageWidth, u32 imageHeight,
                          v2i pos) {
  i32 x = pos.x % imageWidth;
  while (x < 0) x += imageWidth;
  i32 y = pos.y % imageHeight;
  while (y < 0) y += imageHeight;
  return image + (4 * (y * imageWidth + x));
}

static v4 getPixel(u8 *image, u32 imageWidth, u32 imageHeight,
                      v2i pos) {
  u8 *p = getPixelP(image, imageWidth, imageHeight, pos);
  return {(f32)p[0] / 255.f, (f32)p[1] / 255.f, (f32)p[2] / 255.f,
          (f32)p[3] / 255.f};
}

static void setPixel(u8 *image, u32 imageWidth, u32 imageHeight,
                     v2i pos, v4 color) {
  u8 *p = getPixelP(image, imageWidth, imageHeight, pos);
  u8 rgba[4] = {(u8)(color.r * 255.f), (u8)(color.g * 255.f),
                (u8)(color.b * 255.f), (u8)(color.a * 255.f)};
  p[0] = rgba[0];
  p[1] = rgba[1];
  p[2] = rgba[2];
  p[3] = rgba[3];
}

f32 getGeometryHeight(v3 pos, car_game_state *game, v3 *normOut) {
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
  v4 c00 = game->terrain.geometry[sizeX * y + x];
  v4 c10 = game->terrain.geometry[sizeX * y + xPlusOne];
  v4 c01 = game->terrain.geometry[sizeX * yPlusOne + x];
  v4 c11 = game->terrain.geometry[sizeX * yPlusOne + xPlusOne];

  v3 n00 = {c00.y,c00.z,c00.w};
  v3 n10 = {c10.y,c10.z,c10.w};
  v3 n01 = {c01.y,c01.z,c01.w};
  v3 n11 = {c11.y,c11.z,c11.w};

  // Bilinear interpolate
  f32 a = c00.x * (1.f - u) + c10.x * u;
  f32 b = c01.x * (1.f - u) + c11.x * u;
  f32 h = a * (1.f - v) + b * v;

  v3 aN = n00 * (1.f - u) + n10 * u;
  v3 bN = n01 * (1.f - u) + n11 * u;
  v3 normal = aN * (1.f - v) + bN * v;

  *normOut = (v3){normal.x, normal.y, normal.z};
  return pos.z - h;
}

static void updateGeometryMesh(terrain_object *terrain, v3 *meshVertices,
                               v4 *geometry) {
  rt_image_data img = terrain->heightMapImg;
  // NOTE: This assumes that geometry grid vertex count is same
  //       as the heightmap pixel count
  for (i16 y = 0; y < img.height; y++) {
    for (i16 x = 0; x < img.width; x++) {
      v4 height = getPixel((u8 *)img.pixels, img.width, img.height,
                              (v2i){x, y});
      v4 heightA = getPixel((u8 *)img.pixels, img.width, img.height,
                               (v2i){x + 1, y});
      v4 heightB = getPixel((u8 *)img.pixels, img.width, img.height,
                               (v2i){x - 1, y});
      v4 heightC = getPixel((u8 *)img.pixels, img.width, img.height,
                               (v2i){x, y + 1});
      v4 heightD = getPixel((u8 *)img.pixels, img.width, img.height,
                               (v2i){x, y - 1});
      v3 normal  = v3_normalize((v3){height.y, height.z, height.w} * 2.f - (v3){1.f, 1.f, 1.f});
      v3 normalA = v3_normalize((v3){heightA.y, heightA.z, heightA.w} * 2.f - (v3){1.f, 1.f, 1.f});
      v3 normalB = v3_normalize((v3){heightB.y, heightB.z, heightB.w} * 2.f - (v3){1.f, 1.f, 1.f});
      v3 normalC = v3_normalize((v3){heightC.y, heightC.z, heightC.w} * 2.f - (v3){1.f, 1.f, 1.f});
      v3 normalD = v3_normalize((v3){heightD.y, heightD.z, heightD.w} * 2.f - (v3){1.f, 1.f, 1.f});

      v3 v = meshVertices[(y * img.width) + x];

      // Smooth the geometry and normals a bit
      v.z = (height.x + heightA.x + heightB.x + heightC.x + heightD.x) / 5.f;
      v.z = v.z * heightMapScale.z - heightMapScale.z;
      v3 n = (normal + normalA + normalB + normalC + normalD) / 5.f;

      meshVertices[(y * img.width) + x] = v;
      geometry[(y * img.width) + x] = (v4){v.z, n.x, n.y, n.z};
    }
  }
}

static f32 calcHeight(u8* pixels, i16 w, i16 h, i16 x, i16 y){
  while (x < 0) x += w;
  while (y < 0) y += h;
  x = x % w;
  y = y % h;
  f32 height = getPixel(pixels, w, h, (v2i){x, y}).x;
  // f32 noise1 = sinf(((f32)y / (f32)w) * PI) * 4.f;
  // height += noise1;

  return height;
}

// Create normal map data from height map and embbed normal value
// as g b a channels.
static void embbedNormalMapData(rt_image_data img) {
  i16 w = img.width, h = img.height;

  for (i16 x = 0; x < w; x++) {
    for (i16 y = 0; y < h; y++) {
      f32 height = calcHeight((u8 *)img.pixels, w, h, x, y);

      f32 l = calcHeight((u8 *)img.pixels, w, h, x - 1, y);
      f32 b = calcHeight((u8 *)img.pixels, w, h, x, y + 1);
      f32 r = calcHeight((u8 *)img.pixels, w, h, x + 1, y);
      f32 t = calcHeight((u8 *)img.pixels, w, h, x, y - 1);

      // Append normals as gba values
      v3 A  = {0, 0, height * heightMapScale.z};
      v3 B1 = {heightMapScale.y, 0, r * heightMapScale.z};
      v3 B2 = {-heightMapScale.y, 0, l * heightMapScale.z};
      v3 C1 = {0, heightMapScale.y, b * heightMapScale.z};
      v3 C2 = {0, -heightMapScale.y, t * heightMapScale.z};
      v3 n = LERP(calcNormal(A, B1, C1), calcNormal(A, B2, C2),0.5);

      // Add normal map values as pixel g,b,a values.
      setPixel((u8 *)img.pixels, w, h, (v2i){x, y},
               {height, (n.x + 1.f) * 0.5f, (n.y + 1.f) * 0.5f,
                (n.z + 1.f) * 0.5f});
    }
  }
}

static void createTerrain(car_game_state *game,
			       memory_arena *permanentArena, memory_arena *tempArena,
			       rt_command_buffer *rendererBuffer) {
  terrain_object *terrain = &game->terrain;

  // Terrain images and samplers
  if (terrain->terrain_model.terrainTexHandle == 0) {
    rt_image_data terrainImgData = platformApi->loadImage("assets/sand_base.png", 4);
    rt_image_data heightMapImg = platformApi->loadImage("assets/cell_noise.png", 4);
    {
      rt_command_create_texture *cmd = rt_pushRenderCommandArray(
        rendererBuffer, create_texture, 2);

      cmd->image[0] = heightMapImg;
      cmd->imageHandle = &terrain->terrain_model.heightMapTexHandle;
      terrain->heightMapImg = heightMapImg;

      cmd++;
      cmd->image[0] = terrainImgData;
      cmd->imageHandle = &terrain->terrain_model.terrainTexHandle;

      embbedNormalMapData(heightMapImg);
    }

    // Terrain mesh
    {
      mesh_data terrainMeshData =
        mesh_GridShape(tempArena, terrainMeshGridSizeX, terrainMeshGridSizeY,
                       terrainMeshCellNumX, terrainMeshCellNumY, false, false,
                       rt_primitive_triangles);

      rt_command_create_vertex_buffer *cmd = rt_pushRenderCommand(
        rendererBuffer, create_vertex_buffer);
      cmd->vertexData = terrainMeshData.vertexData;
      cmd->indexData = terrainMeshData.indices;
      cmd->vertexDataSize = terrainMeshData.vertexDataSize;
      cmd->indexDataSize = terrainMeshData.indexDataSize;
      cmd->isStreamData = false;
      cmd->vertexArrHandle = &terrain->terrain_model.vertexArrayHandle;
      cmd->vertexBufHandle = &terrain->terrain_model.vertexBufferHandle;
      cmd->indexBufHandle = &terrain->terrain_model.indexBufferHandle;

      cmd->vertexAttributes[0].count = 3;
      cmd->vertexAttributes[0].offset = 0;
      cmd->vertexAttributes[0].stride = 3 * sizeof(f32);
      cmd->vertexAttributes[0].type = rt_data_type_f32;

      terrain->terrain_model.elementNum = terrainMeshData.indexNum;
    }
    // Terrain geometry mesh
    {
      mesh_data geometryMeshData = mesh_GridShape(
        tempArena, geometrySize.x, geometrySize.y, heightMapImg.width,
        heightMapImg.height, false, false, rt_primitive_lines);
      updateGeometryMesh(terrain, (v3 *)geometryMeshData.vertexData,
                         terrain->geometry);
      terrain->geometry_model.elementNum = geometryMeshData.indexNum;
      {
        rt_command_create_vertex_buffer *cmd = rt_pushRenderCommand(
          rendererBuffer, create_vertex_buffer);
        cmd->vertexData = geometryMeshData.vertexData;
        cmd->indexData = geometryMeshData.indices;
        cmd->vertexDataSize = geometryMeshData.vertexDataSize;
        cmd->indexDataSize = geometryMeshData.indexDataSize;
        cmd->isStreamData = false;
        cmd->vertexBufHandle = &terrain->geometry_model.vertexBufferHandle;
        cmd->indexBufHandle = &terrain->geometry_model.indexBufferHandle;
        cmd->vertexArrHandle = &terrain->geometry_model.vertexArrayHandle;

        cmd->vertexAttributes[0].count = 3;
        cmd->vertexAttributes[0].offset = 0;
        cmd->vertexAttributes[0].stride = 3 * sizeof(f32);
        cmd->vertexAttributes[0].type = rt_data_type_f32;
      }
    }
  }
  // Terrain pipelines and shaders
  rt_shader_data terrainShader =
    importShader(tempArena, "./assets/shaders/terrain.vs",
                 "./assets/shaders/terrain.fs");
  if (terrain->terrain_model.programHandle == 0) {
    rt_command_create_shader_program *cmd = rt_pushRenderCommand(
      rendererBuffer, create_shader_program);

    cmd->fragmentShaderData = terrainShader.fsData;
    cmd->vertexShaderData = terrainShader.vsData;
    cmd->shaderProgramHandle = &terrain->terrain_model.programHandle;
  }
  else {
    rt_command_update_shader_program *cmd = rt_pushRenderCommand(
      rendererBuffer, update_shader_program);

    cmd->fragmentShaderData = terrainShader.fsData;
    cmd->vertexShaderData = terrainShader.vsData;
    cmd->shaderProgramHandle = &terrain->terrain_model.programHandle;
  }
  // // Terrain geometry shaders and piplines
  rt_shader_data shader =
    importShader(tempArena, "./assets/shaders/mesh_color.vs",
                 "./assets/shaders/mesh_color.fs");
  if (terrain->geometry_model.programHandle == 0) {
    rt_command_create_shader_program *cmd = rt_pushRenderCommand(
      rendererBuffer, create_shader_program);

    cmd->fragmentShaderData = shader.fsData;
    cmd->vertexShaderData = shader.vsData;
    cmd->shaderProgramHandle = &terrain->geometry_model.programHandle;
  }
  else {
    rt_command_update_shader_program *cmd = rt_pushRenderCommand(
      rendererBuffer, update_shader_program);

    cmd->fragmentShaderData = shader.fsData;
    cmd->vertexShaderData = shader.vsData;
    cmd->shaderProgramHandle = &terrain->geometry_model.programHandle;
  }
}

static void renderTerrain(car_game_state *game, memory_arena *tempArena,
			  rt_command_buffer *rendererBuffer,
			  m4x4 view, m4x4 proj) {
  if (isBitSet(game->debug.visibilityState, visibility_state_terrain)) {
    {
      {
        rt_command_apply_bindings *cmd = rt_pushRenderCommand(
            rendererBuffer, apply_bindings);
        cmd->indexBufferHandle = game->terrain.terrain_model.indexBufferHandle;
        cmd->vertexBufferHandle =
            game->terrain.terrain_model.vertexBufferHandle;
        cmd->vertexArrayHandle = game->terrain.terrain_model.vertexArrayHandle;

        cmd->textureBindings[0] = (rt_binding_data){0};
        cmd->textureBindings[0].textureHandle = game->terrain.terrain_model.heightMapTexHandle,
        cmd->textureBindings[0].samplerHandle = game->terrain.terrain_model.heightMapSamplerHandle;
        cmd->textureBindings[1] = (rt_binding_data){0};
        cmd->textureBindings[1].textureHandle = game->terrain.terrain_model.terrainTexHandle,
        cmd->textureBindings[1].samplerHandle = game->terrain.terrain_model.terrainSamplerHandle;
      }
      {
        rt_command_apply_program *cmd = rt_pushRenderCommand(
            rendererBuffer, apply_program);
        cmd->programHandle = game->terrain.terrain_model.programHandle;
        cmd->ccwFrontFace = true;
        cmd->enableBlending = true;
        cmd->enableCull = true;
        cmd->enableDepthTest = true;
      }
      // // Setup and draw outer terrain mesh
      if(1){
        rt_command_apply_uniforms *cmd = rt_pushRenderCommand(
            rendererBuffer, apply_uniforms);
        cmd->shaderProgram = game->terrain.terrain_model.programHandle;
        terrain_vs_uniform_params *vsParams =
            pushType(tempArena, terrain_vs_uniform_params);

        vsParams->mvp = proj * view;
        vsParams->offset = game->camera.position;
        vsParams->gridSize = {terrainMeshGridSizeX, terrainMeshGridSizeY};
        vsParams->gridCells = {terrainMeshCellNumX, terrainMeshCellNumY};

        vsParams->heightMapScale = heightMapScale;
        vsParams->scaleAndFallOf = (v3){5.0f, 1.0f, 0.9f};

        cmd->uniforms[0] = (rt_uniform_data){
            .type = rt_uniform_type_mat4, .name = STR("mvp"), .data = &vsParams->mvp};
        cmd->uniforms[1] = (rt_uniform_data){.type = rt_uniform_type_vec3,
                                           .name = STR("offset"),
                                           .data = &vsParams->offset};
        cmd->uniforms[2] = (rt_uniform_data){.type = rt_uniform_type_vec2,
                                           .name = STR("gridSize"),
                                           .data = &vsParams->gridSize};
        cmd->uniforms[3] = (rt_uniform_data){.type = rt_uniform_type_vec2,
                                           .name = STR("gridCells"),
                                           .data = &vsParams->gridCells};
        cmd->uniforms[4] = (rt_uniform_data){.type = rt_uniform_type_vec3,
                                           .name = STR("heightMapScale"),
                                           .data = &vsParams->heightMapScale};
        cmd->uniforms[5] = (rt_uniform_data){.type = rt_uniform_type_vec3,
                                           .name = STR("scaleAndFallOf"),
                                           .data = &vsParams->scaleAndFallOf};
      }
      if(1){
        rt_command_draw_elements *cmd = rt_pushRenderCommand(
            rendererBuffer, draw_elements);
        cmd->numElement = game->terrain.terrain_model.elementNum;
      }
      // Setup and draw inner terrain mesh
      {
        rt_command_apply_uniforms *cmd = rt_pushRenderCommand(
            rendererBuffer, apply_uniforms);

        terrain_vs_uniform_params *vsParams =
            pushType(tempArena, terrain_vs_uniform_params);
        cmd->shaderProgram = game->terrain.terrain_model.programHandle;

        vsParams->mvp = proj * view;
        vsParams->offset = game->camera.position;
        vsParams->gridSize = {terrainMeshGridSizeX, terrainMeshGridSizeY};
        vsParams->gridCells = {terrainMeshCellNumX, terrainMeshCellNumY};

        vsParams->heightMapScale = heightMapScale;
        vsParams->scaleAndFallOf = (v3){1.0f, 0.5f, 1.0f};

        vsParams->heightMapSamplerId = 0;
        vsParams->terrainTexSamplerId = 1;

        cmd->uniforms[0] = (rt_uniform_data){
            .type = rt_uniform_type_mat4, .name = STR("mvp"), .data = &vsParams->mvp};
        cmd->uniforms[1] = (rt_uniform_data){.type = rt_uniform_type_vec3,
                                           .name = STR("offset"),
                                           .data = &vsParams->offset};
        cmd->uniforms[2] = (rt_uniform_data){.type = rt_uniform_type_vec2,
                                           .name = STR("gridSize"),
                                           .data = &vsParams->gridSize};
        cmd->uniforms[3] = (rt_uniform_data){.type = rt_uniform_type_vec2,
                                           .name = STR("gridCells"),
                                           .data = &vsParams->gridCells};
        cmd->uniforms[4] = (rt_uniform_data){.type = rt_uniform_type_vec3,
                                           .name = STR("heightMapScale"),
                                           .data = &vsParams->heightMapScale};
        cmd->uniforms[5] = (rt_uniform_data){.type = rt_uniform_type_vec3,
                                           .name = STR("scaleAndFallOf"),
                                           .data = &vsParams->scaleAndFallOf};
        cmd->uniforms[6] =
            (rt_uniform_data){.type = rt_uniform_type_int,
                            .name = STR("height_map"),
                            .data = &vsParams->heightMapSamplerId};
        cmd->uniforms[7] =
            (rt_uniform_data){.type = rt_uniform_type_int,
                            .name = STR("terrain_tex"),
                            .data = &vsParams->terrainTexSamplerId};
      }
      {
        rt_command_draw_elements *cmd = rt_pushRenderCommand(
            rendererBuffer, draw_elements);
        cmd->numElement = game->terrain.terrain_model.elementNum;
      }
    }
  }
  if (isBitSet(game->debug.visibilityState, visibility_state_terrain_geometry)) {
    {
      rt_command_apply_program *cmd =
          rt_pushRenderCommand(rendererBuffer, apply_program);
      cmd->programHandle = game->terrain.geometry_model.programHandle;
      cmd->ccwFrontFace = true;
      cmd->enableBlending = true;
      cmd->enableCull = false;
      cmd->enableDepthTest = true;
    }
    {
      rt_command_apply_bindings *cmd =
          rt_pushRenderCommand(rendererBuffer, apply_bindings);
      cmd->indexBufferHandle = game->terrain.geometry_model.indexBufferHandle;
      cmd->vertexBufferHandle = game->terrain.geometry_model.vertexBufferHandle;
      cmd->vertexArrayHandle = game->terrain.geometry_model.vertexArrayHandle;
    }
    {
      rt_command_apply_uniforms *cmd =
          rt_pushRenderCommand(rendererBuffer, apply_uniforms);
      uptr uniformData =
          (uptr)pushSize(tempArena, sizeof(m4x4) + sizeof(v4));
      m4x4 *mvp = (m4x4 *)uniformData;
      m4x4 model = m4x4_translate_make(v4_from_v3(-game->camera.position, 1.f));
      *mvp = proj * view * model;
      uniformData += sizeof(m4x4);
      v4 *color = (v4 *)uniformData;
      *color = {0.0f, 0.4f, 0.0f, 1.0f};

      cmd->uniforms[0] = (rt_uniform_data){
          .type = rt_uniform_type_mat4, .name = STR("mvp"), .data = mvp};
      cmd->uniforms[1] = (rt_uniform_data){
          .type = rt_uniform_type_vec4, .name = STR("color"), .data = color};
      cmd->shaderProgram = game->terrain.geometry_model.programHandle;
    }
    {
      rt_command_draw_elements *cmd =
          rt_pushRenderCommand(rendererBuffer, draw_elements);
      cmd->mode = rt_primitive_lines;
      cmd->numElement = game->terrain.geometry_model.elementNum;
    }
  }
  if (isBitSet(game->debug.visibilityState, visibility_state_geometry_normals)) {
    u32 lineNum = 10;
    u32 lineNumTotal = lineNum * lineNum * 2;
    f32 lineSpace = 2.f;
    v3 *lines = pushArray(tempArena, lineNumTotal, v3);

    u32 idx = 0;
    v3 carPosition = game->car.body.chassis.position - game->camera.position;
    for (u32 x = 0.f; x < lineNum; x++) {
      for (u32 y = 0.f; y < lineNum; y++) {
        v3 pos = {(f32)carPosition.x + x * lineSpace -
                         (f32)lineNum * lineSpace * 0.5f,
                     (f32)carPosition.y + y * lineSpace -
                         (f32)lineNum * lineSpace * 0.5f,
                     0};

        v3 normal;
        f32 depth = getGeometryHeight(pos, game, &normal);
        lines[idx] = (v3){pos.x, pos.y, -depth};
        lines[idx + 1] = (v3){pos.x, pos.y, -depth} +
                         (v3){normal.x, normal.y, normal.z};
        idx += 2;
      }
    }
    m4x4 vpMat = proj * view;
    rt_command_render_simple_lines *cmd =
        rt_pushRenderCommand(rendererBuffer, render_simple_lines);
    cmd->lines = lines;
    cmd->lineNum = lineNumTotal;
    v4 c = (v4){1.0f, 1.0f, 1.0f, 1.0f};
    cmd->color = c;
    cmd->projView = vpMat;
    cmd->model = M4X4_IDENTITY;
  }
}

static void allocTerrain(
  car_game_state *game,
  memory_arena *permanentArea) {
  game->terrain.geometry = pushArray(permanentArea, 
                                     heightMapImgSize.x * heightMapImgSize.y, v4); 
}

static void terrainInit(car_game_state *game,
			memory_arena *permanentArena,
			memory_arena *tempArena,
			rt_command_buffer *rendererBuffer,
			b32 reload,
			f32 assetModTime) {
  if (reload || !game->terrain.initialized) {
    game->terrain.initialized = true;
  }
}
