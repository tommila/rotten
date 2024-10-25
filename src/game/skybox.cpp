#include "all.h"

// TODO: Create skybox by hand, no need to load it from cgltf

static void skyboxCreateModel(game_state *state, memory_arena *permanentArena,
                              memory_arena *tempArena,
                              rt_render_entry_buffer *rendererBuffer,
                              utime assetModTime) {
  skybox_object *skybox = &state->skybox;

  utime modelModTime =
      platformApi->diskIOReadModTime("./assets/models/Skybox/skybox.gltf");
  if (assetModTime < modelModTime) {
    cgltf_data *gltfData =
        gltf_loadFile(tempArena, "./assets/models/Skybox/skybox.gltf");

    mesh_data meshData = {0};
    mesh_material_data matData = {0};
    mat4 meshTransform;
    cgltf_node *node = &gltfData->nodes[0];
    gltf_readMeshData(tempArena, node, "./assets/models/Skybox/", &meshData,
                      &meshTransform);
    gltf_readMatData(node, "./assets/models/Skybox/", &matData);

    skybox->elementNum = meshData.indexNum;
    {
      if (skybox->vertexArrayHandle == 0) {
        rt_render_entry_create_vertex_buffer *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_create_vertex_buffer);
        cmd->vertexData = meshData.vertexData;
        cmd->indexData = meshData.indices;
        cmd->vertexDataSize = meshData.vertexDataSize;
        cmd->indexDataSize = meshData.indexDataSize;
        cmd->isStreamData = false;
        cmd->vertexBufHandle = &skybox->vertexBufferHandle;
        cmd->indexBufHandle = &skybox->indexBufferHandle;
        cmd->vertexArrHandle = &skybox->vertexArrayHandle;

        cmd->vertexAttributes[0] = {.count = 3,
                                    .offset = 0,
                                    .stride = 8 * sizeof(f32),
                                    .type = rt_renderer_data_type_f32},
        cmd->vertexAttributes[1] = {.count = 3,
                                    .offset = 3 * sizeof(f32),
                                    .stride = 8 * sizeof(f32),
                                    .type = rt_renderer_data_type_f32};
        cmd->vertexAttributes[2] = {.count = 2,
                                    .offset = 6 * sizeof(f32),
                                    .stride = 8 * sizeof(f32),
                                    .type = rt_renderer_data_type_f32};

        skybox->elementNum = meshData.indexNum;
      } else {
        rt_render_entry_update_vertex_buffer *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_update_vertex_buffer);
        cmd->vertexData = meshData.vertexData;
        cmd->indexData = meshData.indices;
        cmd->vertexDataSize = meshData.vertexDataSize;
        cmd->indexDataSize = meshData.indexDataSize;
        cmd->vertexBufHandle = skybox->vertexBufferHandle;
        cmd->indexBufHandle = skybox->indexBufferHandle;
      }
    }
    {
      utime texModTime = platformApi->diskIOReadModTime(matData.imagePath);
      if (assetModTime < texModTime) {
        img_data skyboxImgData = importImage(tempArena, matData.imagePath, 4);
        rt_render_entry_create_image *cmd =
            rt_renderer_pushEntry(rendererBuffer, rt_render_entry_create_image);

        cmd->width = skyboxImgData.width;
        cmd->height = skyboxImgData.height;
        cmd->depth = skyboxImgData.depth;
        cmd->components = skyboxImgData.components;
        cmd->pixels = skyboxImgData.pixels;
        cmd->imageHandle = &skybox->texHandle;
      }
    }
  }
  {  // Terrain model pipeline init
    utime vsShaderModTime =
        platformApi->diskIOReadModTime("./assets/shaders/skybox.vs");
    utime fsShaderModTime =
        platformApi->diskIOReadModTime("./assets/shaders/skybox.fs");
    if (assetModTime < vsShaderModTime || assetModTime < fsShaderModTime) {
      shader_data skyboxShader =
          importShader(tempArena, "./assets/shaders/skybox.vs",
                       "./assets/shaders/skybox.fs");
      if (skybox->programHandle == 0) {
        rt_render_entry_create_shader_program *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_create_shader_program);

        cmd->fragmentShaderData = (char *)skyboxShader.fsData;
        cmd->vertexShaderData = (char *)skyboxShader.vsData;

        cmd->shaderProgramHandle = &skybox->programHandle;
      } else {
        rt_render_entry_update_shader_program *cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_update_shader_program);

        cmd->fragmentShaderData = (char *)skyboxShader.fsData;
        cmd->vertexShaderData = (char *)skyboxShader.vsData;
        cmd->shaderProgramHandle = &skybox->programHandle;
      }
    }
  }
}

static void renderSkybox(game_state* game, memory_arena *tempArena,
                       rt_render_entry_buffer *rendererBuffer, mat4s view,
                       mat4s proj) {
  skybox_object *skybox = &game->skybox;
  {
    rt_render_entry_apply_program *cmd =
        rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_program);
    cmd->programHandle = skybox->programHandle;
    cmd->ccwFrontFace = true;
  }
  {
    rt_render_entry_apply_bindings *cmd =
        rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_bindings);
    cmd->indexBufferHandle = skybox->indexBufferHandle;
    cmd->vertexBufferHandle = skybox->vertexBufferHandle;
    cmd->vertexArrayHandle = skybox->vertexArrayHandle;
    cmd->textureBindings[0] =
        (sampler_binding_entry){.textureHandle = game->skybox.texHandle,
                                .samplerHandle = game->skybox.samplerHandle};
  }
  {
    rt_render_entry_apply_uniforms *cmd =
        rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_uniforms);
    cmd->shaderProgram = skybox->programHandle;
    mat4s *vsParams = pushType(tempArena, mat4s);
    mat4s position = glms_translate_make(game->car.body.chassis.position);
    mat4s scale = glms_scale_make({5000.f, 5000.f, 5000.f});
    *vsParams = proj * view * position * scale;
    cmd->uniforms[0] = (uniform_entry){
        .type = uniform_type_mat4, .name = "mvp", .data = vsParams};
  }
  {
    rt_render_entry_draw_elements *cmd =
        rt_renderer_pushEntry(rendererBuffer, rt_render_entry_draw_elements);
    cmd->numElement = skybox->elementNum;
  }
}

static void skyboxInit(game_state *game,
		       memory_arena *permanentArena,
		       memory_arena *tempArena,
		       rt_render_entry_buffer *rendererBuffer,
		       b32 reload, f32 assetModTime) {
  if (reload || !game->skybox.initialized) {
    skyboxCreateModel(game, permanentArena, tempArena, rendererBuffer, assetModTime);
    game->skybox.initialized = true;
  }
}
