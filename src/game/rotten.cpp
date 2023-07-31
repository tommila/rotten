#include "rotten.h"

#include "../ext/cgltf.h"
#include "camera.cpp"
#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"
#include "cglt_import.cpp"
#include "gfx_shapes.h"
#include "stdio.h"
#include "uniform_grid.cpp"

res_texture *importTexture(platform_state *mem, const char *texId,
                           const char *filePath) {
  size_t imageSize;
  res_texture *res = findRes(&mem->memory.texMemBlock, texId, res_texture);
  if (res) {
    stbi_image_free(res->data.pixels);
  } else {
    res = pushSize(&mem->memory.texMemBlock, res_texture);
  }
  void *imageData = mem->api.diskIOReadFile(filePath, &imageSize, true);
  ASSERT(imageData);
  int x, y, n;
  stbi_uc *pixels = stbi_load_from_memory((stbi_uc *)imageData, (int)imageSize,
                                          &x, &y, &n, 0);
  if (pixels == NULL) {
    LOG(LOG_LEVEL_ERROR, "File load error %s", stbi_failure_reason());
    return 0;
  }
  res->data.pixels = pixels;
  res->data.dataSize = x * y * n;
  res->data.width = (size_t)x;
  res->data.height = (size_t)y;
  res->data.depth = 8;
  res->data.components = (uint32_t)n;

  // Data pointer points next res_texture mem area
  stringCopy(texId, res->header.name);

  stringCopy(filePath, res->data.fileInfo.path);
  res->data.fileInfo.modTime = mem->api.diskIOReadModTime(filePath);
  return res;
}

static res_shader *importShader(platform_state *mem, const char *shaderId,
                                const char *filePath) {
  LOG(LOG_LEVEL_INFO, "Load Shader %s\n", filePath);
  res_shader *res = findRes(&mem->memory.shaderMemBlock, shaderId, res_shader);
  if (res) {
    free(res->data.fsData);
    free(res->data.vsData);
  } else {
    res = (res_shader *)pushSize(&mem->memory.shaderMemBlock, res_shader);
  }
  res->header.type = texture;
  char fsFilePath[64], vsFilePath[64];
  CONCAT2(fsFilePath, filePath, ".fs.sc.bin");
  CONCAT2(vsFilePath, filePath, ".vs.sc.bin");

  res->data.fsData =
      mem->api.diskIOReadFile(fsFilePath, &res->data.fsDataSize, true);
  res->data.vsData =
      mem->api.diskIOReadFile(vsFilePath, &res->data.vsDataSize, true);
  if (res->data.fsData == 0 || res->data.vsData == 0) {
    LOG(LOG_LEVEL_ERROR, "Shader Load failed\n");
  }
  stringCopy(shaderId, res->header.name);

  LOG(LOG_LEVEL_INFO, "Shader Loaded\n");
  stringCopy(vsFilePath, res->data.vsFileInfo.path);
  res->data.vsFileInfo.modTime = mem->api.diskIOReadModTime(vsFilePath);

  stringCopy(fsFilePath, res->data.fsVileInfo.path);
  res->data.fsVileInfo.modTime = mem->api.diskIOReadModTime(fsFilePath);
  return res;
}

static res_model *loadCgltfModel(platform_state *memory, const char *modelName,
                                 const char *filePath) {
  char path[128], fileName[64];
  splitFilePath(filePath, path, fileName);

  size_t dataSize = 0;
  void *data = memory->api.diskIOReadFile(filePath, &dataSize, false);
  ASSERT(data);

  cgltf_options options = {};
  cgltf_data *cgltfData = nullptr;

  res_model *res = findRes(&memory->memory.modelMemBlock, modelName, res_model);
  if (!res) {
    res = (res_model *)pushSize(&memory->memory.modelMemBlock, res_model);
  }
  res->header.type = model;
  ASSERT(cgltf_parse(&options, data, dataSize, &cgltfData) ==
         cgltf_result_success);
  ASSERT(cgltf_load_buffers(&options, cgltfData, filePath) ==
         cgltf_result_success);

  ASSERT(importCgltfTextures(memory, cgltfData->textures,
                             cgltfData->textures_count, path));
  ASSERT(importCgltfMaterials(memory, cgltfData->materials,
                              cgltfData->materials_count));

  stringCopy(modelName, res->header.name);

  stringCopy(filePath, res->fileInfo.path);
  res->fileInfo.modTime = memory->api.diskIOReadModTime(filePath);

  for (cgltf_size sceneIndex = 0; sceneIndex < cgltfData->scenes_count;
       ++sceneIndex) {
    cgltf_scene *scene = &cgltfData->scenes[sceneIndex];
    for (cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count;
         ++nodeIndex) {
      cgltf_node *node = scene->nodes[nodeIndex];
      processGltfNode(res, memory, node, modelName);
    }
  }

  return res;
}

static void loadGameContent(platform_state *platform) {
  game_state *world = (game_state *)platform->memory.gameMemBlock.data;
  {  // terrain plane

    res_model *model = loadCgltfModel(
        platform, "Plane", "./assets/models/Plane-glTF/Plane-glTF.gltf");

    ASSERT(platform->api.initMeshBuffers(&model->meshes[0]->data,
                                         &world->terrain.model.meshBuffers));

    ASSERT(platform->api.initMaterialBuffers(
        model->materials[0], &world->terrain.model.materialBuffers));

    res_shader *shader =
        importShader(platform, "Plane", "./assets/shaders/cubes");

    ASSERT(platform->api.initShaderProgram(
        &shader->data, &world->terrain.model.shaderProgram));
  }

  {
    // cube
    res_model *model = loadCgltfModel(
        platform, "Cube", "./assets/models/BoxTextured-glTF/BoxTextured.gltf");

    ASSERT(platform->api.initMeshBuffers(&model->meshes[0]->data,
                                         &world->cube.model.meshBuffers));

    ASSERT(platform->api.initMaterialBuffers(
        model->materials[0], &world->cube.model.materialBuffers));

    res_shader *shader =
        importShader(platform, "Cubes", "./assets/shaders/cubes");

    ASSERT(platform->api.initShaderProgram(&shader->data,
                                           &world->cube.model.shaderProgram));
  }
}

static void checkResourceChanges(res_header *res, model_state *modelState,
                                 platform_state *mem) {
  if (res->type == model) {
    res_model *model = (res_model *)res;
    if (model->fileInfo.modTime <
        mem->api.diskIOReadModTime(model->fileInfo.path)) {
      LOG(LOG_LEVEL_INFO, "Reload glTF model %s\n", model->header.name);
      res_model *newModel =
          loadCgltfModel(mem, model->header.name, model->fileInfo.path);

      // TODO: create vertex data update
      // Free current model buffer data and create new
      mem->api.freeMeshBuffers(&modelState->meshBuffers);
      ASSERT(mem->api.initMeshBuffers(&newModel->meshes[0]->data,
                                      &modelState->meshBuffers));

      ASSERT(mem->api.updateMaterialBuffers(newModel->materials[0],
                                            &modelState->materialBuffers));
    } else if (model->materials[0]->texture->data.fileInfo.modTime <
               mem->api.diskIOReadModTime(
                   model->materials[0]->texture->data.fileInfo.path)) {
      LOG(LOG_LEVEL_INFO, "Reload texture %s\n",
          model->materials[0]->texture->header.name);
      importTexture(mem, model->materials[0]->texture->header.name,
                    model->materials[0]->texture->data.fileInfo.path);
      ASSERT(mem->api.updateMaterialBuffers(model->materials[0],
                                            &modelState->materialBuffers));
    }
  }
}

extern "C" GAME_ON_START(gameOnStart) {
  // Convience method bindings
  ASSERT_ = platform->api.assert;
  LOG_ = platform->api.log;

  // TODO: clean up memory organization
  uint8_t *memP = (uint8_t *)platform->permanentMemory.data;
  platform->memory.texMemBlock = {memP, 0, MEGABYTES(4)};

  memP += platform->memory.texMemBlock.size;
  platform->memory.matMemBlock = {memP, 0, MEGABYTES(4)};

  memP += platform->memory.matMemBlock.size;
  platform->memory.meshMemBlock = {memP, 0, MEGABYTES(4)};

  memP += platform->memory.meshMemBlock.size;
  platform->memory.shaderMemBlock = {memP, 0, MEGABYTES(4)};

  memP += platform->memory.shaderMemBlock.size;
  platform->memory.modelMemBlock = {memP, 0, MEGABYTES(4)};

  memP += platform->memory.modelMemBlock.size;
  platform->memory.gameMemBlock = {memP, 0, MEGABYTES(4)};

  platform->api.initDisplay(1920, 1080);

  loadGameContent(platform);

  vec3 eye = {0.0f, 0.0f, 35.0f};
  vec3 dir = {0.0f, 0.0f, -1.0f};
  vec3 up = {0.0f, 1.0f, 0.0f};

  game_state *world = (game_state *)platform->memory.gameMemBlock.data;

  glm_vec3_copy(eye, world->camera.eye);
  glm_vec3_copy(dir, world->camera.dir);
  glm_vec3_copy(up, world->camera.up);
  world->camera.aspectRatio = float(1920) / float(1080);
  world->camera.moveSpeed = 30.0f;
  world->camera.zoomIdx = 3;
}

float fileWatchTime = 0;

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {
  game_state *world = (game_state *)platform->memory.gameMemBlock.data;

  fileWatchTime += delta;
  if (fileWatchTime > 1) {
    fileWatchTime = 0;
    checkResourceChanges((res_header *)findRes(&platform->memory.modelMemBlock,
                                               "Plane", res_model),
                         &world->terrain.model, platform);
  }

  bool quit = false;
  platform->temporaryMemory.head = 0;
  if (input->quitButton.state >= Pressed) {
    quit = input->quitButton.state >= Pressed;
  }

  /// Simulation ///
  if (input->gridButton.state == Pressed) {
    world->debug.drawGrid = !world->debug.drawGrid;
  }

  if (input->pauseButton.state == Pressed) {
    world->debug.pause = !world->debug.pause;
  }
  bool step = input->timeStepButton.state >= Pressed;
  float dt = delta * float(!world->debug.pause || step);

  /// Renderering ///
  vec2 cameraMoveDir = GLM_VEC2_FROM(input->axis_state);
  glm_vec2_normalize(cameraMoveDir);
  cameraMove(dt, cameraMoveDir, &world->camera);

  mat4 viewMat = GLM_MAT4_ZERO_INIT;
  mat4 projMat = GLM_MAT4_ZERO_INIT;

  cameraZoom(input->wheel, &world->camera);
  cameraGetProjectionMat(&projMat, &world->camera);
  cameraGetViewMat(&viewMat, &world->camera);

  mat4 planeMat = GLM_MAT4_IDENTITY_INIT;
  vec3 planeScale = {15.f, 15.f, 1.0f};
  glm_scale_make(planeMat, planeScale);

  mat4 cubeMat = GLM_MAT4_IDENTITY_INIT;
  vec3 cubeScale = {(sin(duration * 0.001f) + 2.0f) * 2.0f,
                    (cos(duration * 0.001f) + 2.0f) * 2.0f,
                    (sin(duration * 0.001f) + 2.0f) * 2.0f};
  vec3 cubeOffset = {1.0f, 1.0, 1.0f};
  glm_translate_make(cubeMat, cubeOffset);
  glm_scale(cubeMat, cubeScale);
  vec3 axis = {sin(duration * 0.0001f), -1.f, cos(duration * 0.0001f)};
  glm_rotate(cubeMat, duration * 0.001, axis);
  platform->api.renderBegin(&viewMat, &projMat);

  platform->api.renderMesh(&world->terrain.model.meshBuffers,
                           &world->terrain.model.materialBuffers,
                           &world->terrain.model.shaderProgram, planeMat);

  platform->api.renderMesh(&world->cube.model.meshBuffers,
                           &world->cube.model.materialBuffers,
                           &world->cube.model.shaderProgram, cubeMat);

  platform->api.renderEnd();

  if (quit) {
    platform->api.freeMeshBuffers(&world->terrain.model.meshBuffers);
    platform->api.freeMaterialBuffers(&world->terrain.model.materialBuffers);
    platform->api.freeShaderProgram(&world->terrain.model.shaderProgram);
  }

  return quit;
}
