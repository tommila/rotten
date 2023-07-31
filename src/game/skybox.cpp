#include "rotten.h"

static void deleteSkybox(renderer_command_buffer *rendererBuffer,
                         game_state *game) {
  skybox_object *skybox = &game->skybox;
  // Terrain
  {
    renderer_command_free_image *cmd =
        renderer_pushCommand(rendererBuffer, renderer_command_free_image);
    cmd->imageHandle = skybox->texHandle;
    skybox->texHandle = 0;
  }
  {
    renderer_command_free_sampler *cmd =
        renderer_pushCommand(rendererBuffer, renderer_command_free_sampler);
    cmd->samplerHandle = skybox->samplerHandle;

    skybox->samplerHandle = 0;
  }
  {
    renderer_command_free_vertex_buffer *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_vertex_buffer);
    cmd->vertexBufferHandle = skybox->vertexBufferHandle;
    cmd->indexBufferHandle = skybox->indexBufferHandle;

    skybox->vertexBufferHandle = 0;
    skybox->indexBufferHandle = 0;
  }
  {
    renderer_command_free_program_pipeline *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_program_pipeline);
    cmd->pipelineHandle = skybox->pipelineHandle;
    cmd->shaderProgramHandle = skybox->programHandle;

    skybox->pipelineHandle = 0;
    skybox->programHandle = 0;
  }
}

static void createSkybox(memory_arena *permanentArena, memory_arena *tempArena,
                          renderer_command_buffer *rendererBuffer,
                          game_state *state) {
  skybox_object *skybox = &state->skybox;

  cgltf_data *gltfData = gltf_loadFile(tempArena, "./assets/models/Skybox/skybox.gltf");

  mesh_data meshData = {0};
  material_data matData = {0};
  mat4 meshTransform;
  cgltf_node *node = &gltfData->nodes[0];
  gltf_readMeshData(
      tempArena, node, "./assets/models/Skybox/", &meshData, &meshTransform);
  gltf_readMatData(node, "./assets/models/Skybox/", &matData);

  skybox->elementNum = meshData.indexNum;
  skybox->vertexBufferLayout = {0};
  {
    renderer_command_create_vertex_buffer *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_create_vertex_buffer);
    cmd->vertexData = meshData.vertexData;
    cmd->indexData = meshData.indices;
    cmd->vertexDataSize =
        sizeof(f32) *
      (meshData.vertexNum +
       meshData.normalNum +
       meshData.texCoordNum +
       meshData.colorNum);
    cmd->indexDataSize = sizeof(u32) * meshData.indexNum;
    cmd->isStreamData = false;
    cmd->vertexBufHandle = &skybox->vertexBufferHandle;
    cmd->indexBufHandle = &skybox->indexBufferHandle;

    skybox->elementNum = meshData.indexNum;
  }
  {
    img_data skyboxImgData = importImage(tempArena, matData.imagePath, 4);
    renderer_command_create_image *cmd =
        renderer_pushCommand(rendererBuffer, renderer_command_create_image);

    cmd->width = skyboxImgData.width;
    cmd->height = skyboxImgData.height;
    cmd->depth = skyboxImgData.depth;
    cmd->components = skyboxImgData.components;
    cmd->dataSize = skyboxImgData.dataSize;
    cmd->pixels = skyboxImgData.pixels;
    cmd->imageHandle = &skybox->texHandle;
  }
  {
    renderer_command_create_sampler *cmd =
        renderer_pushCommand(rendererBuffer, renderer_command_create_sampler);
    cmd->samplerHandle = &skybox->samplerHandle;
  }
  {  // Terrain model pipeline init
    shader_data skyboxShader = importShader(
        tempArena, "./assets/shaders/skybox.vs",
	"./assets/shaders/skybox.fs");

    renderer_command_create_program_pipeline *cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_create_program_pipeline);

    cmd->fragmentShaderData = (char *)skyboxShader.fsData;
    cmd->vertexShaderData = (char *)skyboxShader.vsData;
    cmd->fragmentSamplerEntries = rt_renderer_allocSamplerBuffer(tempArena, 1);
    rt_renderer_pushSamplerEntry(&cmd->fragmentSamplerEntries, "tex");

    cmd->layout = {
        .position = {.format = vertex_format_float3,
                     .offset = 0},
        .normals = {.format = vertex_format_float3,
                    .offset = sizeof(f32) * 3},
        .uv = {.format = vertex_format_float2,
	       .offset = sizeof(f32) * 6},
        .color = {.layoutIdx = -1}};
    cmd->depthTest = true;
    cmd->shaderProgramHandle = &skybox->programHandle;
    cmd->progPipelineHandle = &skybox->pipelineHandle;

    cmd->vertexUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 8);
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries, mat4,
                                 "mvp");

    skybox->vsUniformSize = cmd->vertexUniformEntries.bufferSize;
  }
}

static void drawSkybox(memory_arena *tempArena, game_state *state,
		       renderer_command_buffer *rendererBuffer,
		       mat4s view, mat4s proj, vec3s carPosition) {
  skybox_object *skybox = &state->skybox;
  {
    renderer_command_apply_program_pipeline *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_program_pipeline);
    cmd->pipelineHandle = skybox->pipelineHandle;
  }
  {
        renderer_command_apply_bindings *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_apply_bindings);
        cmd->indexBufferHandle = skybox->indexBufferHandle;
        cmd->vertexBufferHandle =
            skybox->vertexBufferHandle;
        cmd->bufferLayout = skybox->vertexBufferLayout;
	cmd->fragmentSamplerBindings = rt_renderer_allocSamplerBindingBuffer(tempArena, 4);
        rt_renderer_pushSamplerBindingEntry(
            &cmd->fragmentSamplerBindings,
            skybox->samplerHandle,
            skybox->texHandle);
  }
  {
    renderer_command_apply_uniforms *cmd = renderer_pushCommand(
								rendererBuffer, renderer_command_apply_uniforms);

    cmd->vertexUniformDataBuffer = rt_renderer_allocUniformDataBuffer(
								      tempArena, sizeof(mat4s));
        mat4s *vsParams =
            (mat4s *)cmd->vertexUniformDataBuffer.buffer;
	mat4s position = glms_translate_make(carPosition);
	mat4s scale = glms_scale_make({5000.f,5000.f,5000.f});
        *vsParams = proj * view * position * scale;
	cmd->vertexUniformDataBuffer.bufferSize = sizeof(mat4s);
  }
  {
        renderer_command_draw_elements *cmd = renderer_pushCommand(
            rendererBuffer, renderer_command_draw_elements);
        cmd->baseElement = 0;
        cmd->numElement = skybox->elementNum;
        cmd->numInstance = 1;
  }
}
