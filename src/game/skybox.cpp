#include "all.h"

// TODO: Create skybox by hand, no need to load it from cgltf

f32 skyboxCubeVerts[] = {
   1.0,  1.0,  1.0,
   1.0, -1.0,  1.0,
  -1.0, -1.0,  1.0,
  -1.0,  1.0,  1.0,
  
   1.0,  1.0, -1.0,
   1.0, -1.0, -1.0,
  -1.0, -1.0, -1.0,
  -1.0,  1.0, -1.0,
};

u32 skyboxCubeIndices[] = {
  // UP
  0, 1, 2,  
  2, 3, 0,
  // BOTTOM
  4, 5, 6,  
  6, 7, 4,
  // FRONT
  0, 4, 7, 
  7, 3, 0,
  // BACK
  1, 5, 6,  
  6, 2, 1,
  // LEFT
  3, 2, 6, 
  6, 7, 3,
  // RIGHT
  0, 1, 5,
  5, 4, 0
};

static void createSkyBox(car_game_state *game, memory_arena *permanentArena,
                              memory_arena *tempArena,
                              rt_command_buffer *rendererBuffer) {
  if (game->skybox.vertexArrayHandle == 0) {
    rt_command_create_vertex_buffer *cmd = rt_pushRenderCommand(
      rendererBuffer, create_vertex_buffer);
    cmd->vertexData = skyboxCubeVerts;
    cmd->indexData = skyboxCubeIndices;
    cmd->vertexDataSize = sizeof(skyboxCubeVerts);
    cmd->indexDataSize = sizeof(skyboxCubeIndices);
    cmd->isStreamData = false;
    cmd->vertexBufHandle = &game->skybox.vertexBufferHandle;
    cmd->indexBufHandle = &game->skybox.indexBufferHandle;
    cmd->vertexArrHandle = &game->skybox.vertexArrayHandle;

    cmd->vertexAttributes[0].count = 3;
    cmd->vertexAttributes[0].offset = 0;
    cmd->vertexAttributes[0].stride = 3 * sizeof(f32);
    cmd->vertexAttributes[0].type = rt_data_type_f32;

    game->skybox.elementNum = arrayLen(skyboxCubeIndices);
  }
  if (game->skybox.texHandle == 0){
    rt_command_create_texture *cmd =
      rt_pushRenderCommand(rendererBuffer, create_texture);

    cmd->image[0] = platformApi->loadImage("assets/bluecloud_ft.jpg", 3);
    cmd->image[1] = platformApi->loadImage("assets/bluecloud_bk.jpg", 3);
    cmd->image[2] = platformApi->loadImage("assets/bluecloud_up.jpg", 3);
    cmd->image[3] = platformApi->loadImage("assets/bluecloud_dn.jpg", 3);
    cmd->image[4] = platformApi->loadImage("assets/bluecloud_rt.jpg", 3);
    cmd->image[5] = platformApi->loadImage("assets/bluecloud_lf.jpg", 3);
    cmd->textureType = rt_texture_type_cubemap;
    cmd->imageHandle = &game->skybox.texHandle;
  }
  { 
    rt_shader_data skyboxShader =
      importShader(tempArena, "./assets/shaders/skybox.vs",
                   "./assets/shaders/skybox.fs");
    if (game->skybox.programHandle == 0) {
      rt_command_create_shader_program *cmd = rt_pushRenderCommand(
        rendererBuffer, create_shader_program);

      cmd->fragmentShaderData = skyboxShader.fsData;
      cmd->vertexShaderData = skyboxShader.vsData;

      cmd->shaderProgramHandle = &game->skybox.programHandle;
    } else {
      rt_command_update_shader_program *cmd = rt_pushRenderCommand(
        rendererBuffer,update_shader_program);

      cmd->fragmentShaderData = skyboxShader.fsData;
      cmd->vertexShaderData = skyboxShader.vsData;
      cmd->shaderProgramHandle = &game->skybox.programHandle;
    }
  }
}

static void renderSkybox(car_game_state* game, memory_arena *tempArena,
                       rt_command_buffer *rendererBuffer, m4x4 view,
                       m4x4 proj) {
  {
    rt_command_apply_bindings *cmd =
        rt_pushRenderCommand(rendererBuffer, apply_bindings);
    cmd->indexBufferHandle = game->skybox.indexBufferHandle;
    cmd->vertexBufferHandle = game->skybox.vertexBufferHandle;
    cmd->vertexArrayHandle = game->skybox.vertexArrayHandle;
    cmd->textureBindings[0] =
        (rt_binding_data){
        .textureHandle = game->skybox.texHandle,
        .textureType = rt_texture_type_cubemap,
        .samplerHandle = game->skybox.samplerHandle};
  }
  {
    rt_command_apply_program *cmd =
        rt_pushRenderCommand(rendererBuffer, apply_program);
    cmd->programHandle = game->skybox.programHandle;
    cmd->enableCull = false;
  }
  {
    rt_command_apply_uniforms *cmd =
        rt_pushRenderCommand(rendererBuffer, apply_uniforms);
    cmd->shaderProgram = game->skybox.programHandle;
    m4x4 *vsParams = pushType(tempArena, m4x4);
    *vsParams = proj * view;
    cmd->uniforms[0] = (rt_uniform_data){
        .type = rt_uniform_type_mat4,
        .name = STR("mvp"),
        .data = vsParams};
    cmd->uniforms[1] = (rt_uniform_data){
        .type = rt_uniform_type_int,
        .name = STR("tex"),
        .data = &game->skybox.samplerHandle};
  }
  {
    rt_command_draw_elements *cmd =
        rt_pushRenderCommand(rendererBuffer, draw_elements);
    cmd->numElement = game->skybox.elementNum;
  }
}

