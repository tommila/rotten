#include "rotten.h"

#include "cglm/affine.h"
#include "cglm/euler.h"

#include "../core/mem_arena.c"
#include "../core/mem_free_list.c"

static memory_arena *stack;

static void* _malloc(usize size) { return memArena_alloc(stack, size);}
static void* _realloc(void* ptr, usize newSize) {
  return memArena_realloc(stack, ptr, newSize);
}
// Temp memory is freed every frame
static void _free(void* ptr){}

#define STBI_MALLOC(size) _malloc(size)
#define STBI_REALLOC(p, newSize) _realloc(p, newSize)
#define STBI_FREE(p) _free(p)

#define CGLTF_MALLOC(size) _malloc(size)
#define CGLTF_FREE(p) _free(p)

#define STB_IMAGE_IMPLEMENTATION
#include "../common/math.h"
#include "../ext/stb_image.h"
#include "shapes.c"
#include "gltf_import.cpp"
#include "ui.cpp"
#include "mesh_shape.c"
#include "car.cpp"
#include "terrain.cpp"
#include "skybox.cpp"

inline img_data importImage(memory_arena *arena, const char *filePath, uint8_t channelNum) {
  img_data imgData = {};
  void *data;
  u32 fileSize;
  loadFile(arena, filePath, true, data, &fileSize);
  int x, y, n;
  stbi_uc *pixels = stbi_load_from_memory(
      (stbi_uc *)data, (i32)fileSize, &x, &y, &n, channelNum);
  if (pixels == NULL) {
    LOG(LOG_LEVEL_ERROR, "File load error %s", stbi_failure_reason());
    ASSERT(0);
  }

  imgData.pixels = pixels;
  imgData.dataSize = x * y * 4;
  imgData.width = x;
  imgData.height = y;
  imgData.depth = 8;
  imgData.components = (u32)4;

  return imgData;
}

inline shader_data importShader(memory_arena *arena, const char *shaderVsFilePath,
                         const char *shaderFsFilePath) {
  shader_data shaderData = {0};

  u32 _unused = 0;
  loadFile(arena, shaderVsFilePath, false, shaderData.vsData, &_unused);
  loadFile(arena, shaderFsFilePath, false, shaderData.fsData, &_unused);

  if (shaderData.fsData == 0 || shaderData.vsData == 0) {
    LOG(LOG_LEVEL_ERROR, "Shader Load failed\n");
  }
  LOG(LOG_LEVEL_DEBUG, "Shader Loaded\n");

  return shaderData;
}

static float fileWatchTime = 0;

extern "C" RT_GAME_UPDATE_AND_RENDER(gameUpdate) {
  duration += delta;

  platformApi = &platform->api;
  ASSERT_ = platformApi->assert;
  LOG = platformApi->logger;
  MALLOC = _malloc;
  REALLOC = _realloc;
  FREE = _free;

  u64 totalStartCount = platformApi->getPerformanceCounter();

  memory_arena tempMemory;
  memory_arena permanentMemory;

  memArena_init(&tempMemory, platform->transientMemBuffer, platform->transientMemSize);
  memArena_init(&permanentMemory, platform->permanentMemBuffer, platform->permanentMemSize);

  stack = &tempMemory;

  game_state *gameState = (game_state*)memArena_alloc(&permanentMemory, sizeof(game_state));

  renderer_command_buffer rendererBuffer;
  memArena_init(&rendererBuffer.arena,
		memArena_alloc(&tempMemory, KILOBYTES(256)), KILOBYTES(256));
  nk_context* ctx = rt_nuklear_alloc(&permanentMemory, &tempMemory);

  if (gameState->initialized == false) {
    renderer_command_setup* setupCmd = renderer_pushCommand(&rendererBuffer, renderer_command_setup);
    setupCmd->allocFunc = _malloc;
    setupCmd->freeFunc = _free;
    setupCmd->log = platformApi->logger;
    setupCmd->assert = platformApi->assert;

    LOG(LOG_LEVEL_DEBUG, "terrain loaded");

    gameState->camera.fov = 60;
    gameState->camera.position = {-30.0, 25.0, -30.0};
    gameState->camera.front = {0.0, 0.0, 1.0};
    gameState->camera.yaw = 0.f;
    gameState->camera.pitch = -50.f;
    gameState->initialized = true;
    gameState->assetModTime = platformApi->diskIOReadModTime("assets/modfile");

    gameState->deve.drawState = DRAW_CAR | DRAW_TERRAIN;
    gameState->deve.drawDevePanel = true;

    createSkybox(&permanentMemory, &tempMemory, &rendererBuffer, gameState);
    createTerrain(&permanentMemory, &tempMemory,&rendererBuffer, gameState);
    createCar(&permanentMemory, &tempMemory, &rendererBuffer, gameState);
    ctx = rt_nuklear_setup(&permanentMemory, &tempMemory, &rendererBuffer, LOG, ASSERT_);

    platformApi->renderer_flushCommandBuffer(&rendererBuffer);
  }

  if (reloaded) {
    ctx = rt_nuklear_setup(&permanentMemory, &tempMemory, &rendererBuffer, LOG, ASSERT_);
  }

  for (u32 i = 0; i < arrayLen(gameState->input.buttons); i++) {
    button_state b = gameState->input.buttons[i];
    gameState->input.buttons[i] =
      (b == BUTTON_PRESSED) ? BUTTON_HELD
      : (b == BUTTON_RELEASED) ? BUTTON_UP
      : b;
  }

  for (u32 i = 0; i < arrayLen(gameState->input.mouse.button); i++) {
    button_state b = gameState->input.mouse.button[i];
    gameState->input.mouse.button[i] =
      (b == BUTTON_PRESSED) ? BUTTON_HELD
      : (b == BUTTON_RELEASED) ? BUTTON_UP
      : b;
  }
  gameState->input.mouse.movement[0] = 0;
  gameState->input.mouse.movement[1] = 0;

  for (u32 i = 0; i < INPUT_BUFFER_SIZE; i++) {
    rt_input_event* input = inputEvents + i;
    if (input->type == RT_INPUT_TYPE_NONE) {
      break;
    }
    if (input->type == RT_INPUT_TYPE_KEY) {
      rt_key_event *keyEvt = &input->key;
      if (strcmp(keyEvt->keycode, "W") == 0) {
	gameState->input.camMoveUp = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "S") == 0) {
	gameState->input.camMoveDown = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "A") == 0) {
	gameState->input.camMoveLeft = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "D") == 0) {
	gameState->input.camMoveRight = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "Up") == 0) {
	gameState->input.moveUp = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "Down") == 0) {
	gameState->input.moveDown = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "Left") == 0) {
	gameState->input.moveLeft = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "Right") == 0) {
	gameState->input.moveRight = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "R") == 0) {
	gameState->input.reset = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "Escape") == 0) {
	gameState->input.quit = setButtonState(keyEvt->pressed);
      }
      else if (strcmp(keyEvt->keycode, "C") == 0 && keyEvt->pressed) {
	gameState->deve.freeCameraView = !gameState->deve.freeCameraView;
      }
      else if (strcmp(keyEvt->keycode, "F5") == 0 && keyEvt->pressed) {
        gameState->deve.drawDevePanel = !gameState->deve.drawDevePanel;
      }
    }
    else if(input->type == RT_INPUT_TYPE_MOUSE) {
      rt_mouse_event *mouse = &input->mouse;
      for (u32 i = 0; i < arrayLen(gameState->input.mouse.button); i++) {
	if (mouse->buttonIdx) {
	  gameState->input.mouse.button[mouse->buttonIdx - 1] =
	    setButtonState(mouse->pressed);
	}
	else {

	  gameState->input.mouse.movement[0] +=
	    (i32)mouse->position[0] - (i32)gameState->input.mouse.position[0];
	  gameState->input.mouse.movement[1] +=
	    (i32)mouse->position[1] - (i32)gameState->input.mouse.position[1];
	  gameState->input.mouse.position[0] = mouse->position[0];
	  gameState->input.mouse.position[1] = mouse->position[1];
	  gameState->input.mouse.wheel = mouse->wheel;
	}
      }
    }
  }

  b32 filesChanged = false;
  fileWatchTime += delta;
  if (fileWatchTime > 1) {
    fileWatchTime = 0;
    time_t assetFileModTime = platformApi->diskIOReadModTime("assets/modfile");
    filesChanged = gameState->assetModTime < assetFileModTime;
    if (filesChanged) {
      gameState->assetModTime = assetFileModTime;
      deleteSkybox(&rendererBuffer, gameState);
      deleteCar(&rendererBuffer, gameState);
      deleteTerrain(&rendererBuffer, gameState);

      platformApi->renderer_flushCommandBuffer(&rendererBuffer);

      createSkybox(&permanentMemory, &tempMemory,&rendererBuffer, gameState);
      createCar(&permanentMemory, &tempMemory,&rendererBuffer, gameState);
      createTerrain(&permanentMemory, &tempMemory,&rendererBuffer, gameState);

      platformApi->renderer_flushCommandBuffer(&rendererBuffer);
    }
  }
  if (gameState->deve.drawDevePanel) {
    ui_draw(ctx, gameState);
  }

  b32 quit = gameState->input.quit;

  /// Simulation ///
  mat4s viewProjM = GLMS_MAT4_IDENTITY_INIT;
  vec3s camTarget = gameState->car.body.chassis.position -
    gameState->car.body.chassis.localCenter;
  camera_state *cam = &gameState->camera;
  cam->fov = CLAMP(cam->fov - gameState->input.mouse.wheel * 5, 45, 90);
  mat4s projM = glms_perspective(glm_rad(cam->fov), float(1280) / float(840),
				 0.1f, 10000.0f);
  mat4s viewM = GLMS_MAT4_IDENTITY_INIT;

  if (gameState->input.mouse.button[0]) {
    cam->yaw -= gameState->input.mouse.movement[0] * 0.1f;
    cam->pitch += gameState->input.mouse.movement[1] * 0.1f;
  }
  /// Free view camera
  if (gameState->deve.freeCameraView) {
    vec3s forward = {sinf(glm_rad(cam->yaw)) * -sinf(glm_rad(cam->pitch)),
                     -cosf(glm_rad(cam->yaw)), cosf(glm_rad(cam->pitch))};

    vec3s right = {sinf(glm_rad(cam->yaw - 90.f)),
                   -cosf(glm_rad(cam->yaw - 90.f)), 0.0f};

    cam->front = glms_vec3_normalize(forward);
    cam->right = glms_vec3_normalize(right);

    f32 camSpeed = 50.0f * delta;
    if (gameState->input.camMoveUp >= BUTTON_PRESSED) {
      cam->position = cam->position + cam->front * camSpeed;
    }
    if (gameState->input.camMoveDown >= BUTTON_PRESSED) {
      cam->position = cam->position - cam->front * camSpeed;
    }
    if (gameState->input.camMoveLeft >= BUTTON_PRESSED) {
      cam->position = cam->position - cam->right * camSpeed;
    }
    if (gameState->input.camMoveRight >= BUTTON_PRESSED) {
      cam->position = cam->position + cam->right * camSpeed;
    }

    /// Projection and view matrices ///
    mat4s pitch = glms_rotate_x(GLMS_MAT4_IDENTITY, glm_rad(cam->pitch));
    mat4s yaw = glms_rotate_z(pitch, -glm_rad(cam->yaw));
    viewM = glms_translate_make(cam->position);
    viewM = yaw * viewM;
  }
  // 3rd person car camera
  else {
    vec3s pos = camTarget - gameState->car.body.chassis.orientation *
      glms_vec3_rotate((vec3s){8.f,0.f,-5.f}, cam->yaw * 0.1f, GLMS_ZUP);
    // vec3s pos = camTarget - gameState->car.body.chassis.orientation *
    //   (vec3s){10.f,0.f,-10.f};
    cam->position = LERP(cam->position, pos, 0.05f);
    viewM = glms_lookat(cam->position, camTarget, GLMS_ZUP);
    // viewM = glms_rotate_make(a, glms_vec3_cross(GLMS_YUP, glms_vec3_cross(dir, GLMS_ZUP))) * viewM;
  }

  viewProjM = projM * viewM;
  u64 simStartCount = platformApi->getPerformanceCounter();
  simulateCar(gameState, &gameState->input, delta);
  gameState->profiler.simulationAcc +=
      platformApi->getPerformanceCounter() - simStartCount;

  renderer_command_clear* clearCmd =
    renderer_pushCommand(&rendererBuffer, renderer_command_clear);
  clearCmd->clearColor[0] = 0.6f;
  clearCmd->clearColor[1] = 0.7f;
  clearCmd->clearColor[2] = 0.6f;
  clearCmd->clearColor[3] = 1.0f;
  clearCmd->width = 1920;
  clearCmd->height = 1080;

  drawSkybox(&tempMemory, gameState, &rendererBuffer, viewM, projM, camTarget);
  drawTerrain(&tempMemory, gameState, &rendererBuffer, viewM, projM, camTarget);
  drawCar(gameState, &tempMemory, &rendererBuffer, viewM, projM);

  rt_nuklear_handle_input(inputEvents);
  ctx = rt_nuklear_draw(&tempMemory, &rendererBuffer, platformApi);

  renderer_command_flip* flipCmd = renderer_pushCommand(&rendererBuffer, renderer_command_flip);
  flipCmd->width = 1920;
  flipCmd->height = 1080;

  u64 rendererStartCount = platformApi->getPerformanceCounter();
  platformApi->renderer_flushCommandBuffer(&rendererBuffer);
  gameState->profiler.renderingAcc +=
      platformApi->getPerformanceCounter() - rendererStartCount;

  gameState->profiler.totalAcc +=
      platformApi->getPerformanceCounter() - totalStartCount;

  gameState->profiler.elapsedTime += delta;
  if (gameState->profiler.elapsedTime > 1.f) {
    u64 freq = (f64)platformApi->getPerformanceFrequency();
    gameState->profiler.elapsedTime = gameState->profiler.elapsedTime - 1.f;
    gameState->profiler.rendering =
        1000.f * (f64)gameState->profiler.renderingAcc / 60.f / (f64)freq;

    gameState->profiler.simulation =
        1000.f * (f64)gameState->profiler.simulationAcc / 60.f / (f64)freq;

    gameState->profiler.total =
        1000.f * (f64)gameState->profiler.totalAcc / 60.f / (f64)freq;

    gameState->profiler.renderingAcc = 0;
    gameState->profiler.simulationAcc = 0;
    gameState->profiler.totalAcc = 0;
  };

  return quit;
}
