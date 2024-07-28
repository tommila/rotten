#include "all.h"

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

  game_state *game = (game_state*)memArena_alloc(&permanentMemory, sizeof(game_state));

  rt_render_entry_buffer rendererBuffer;
  memArena_init(&rendererBuffer.arena,
		memArena_alloc(&tempMemory, KILOBYTES(256)), KILOBYTES(256));
  nk_context* ctx = rt_nuklear_alloc(&permanentMemory, &tempMemory);

  b32 initialize = !game->initialized || reloaded;

  // Initial component initialization
  if (initialize) {
    LOG(LOG_LEVEL_DEBUG, "terrain loaded");
    if (!game->initialized) {
      game->camera.fov = 90;
      game->camera.position = {-30.0, 25.0, -30.0};
      game->camera.front = {0.0, 0.0, 1.0};
      game->camera.yaw = 0.f;
      game->camera.pitch = -50.f;

      game->deve.drawState = DRAW_CAR | DRAW_TERRAIN;
      game->deve.drawDevePanel = true;
      game->initialized = true;
    }
    ctx = rt_nuklear_setup(&permanentMemory, &tempMemory, &rendererBuffer, LOG,
                           ASSERT_);

    skyboxInit(game, &permanentMemory, &tempMemory, &rendererBuffer, reloaded,
               assetModTime);
    terrainInit(game, &permanentMemory, &tempMemory, &rendererBuffer, reloaded,
                assetModTime);
    carInit(game, &permanentMemory, &tempMemory, &rendererBuffer, reloaded,
            assetModTime);
    platformApi->renderer_flushCommandBuffer(&rendererBuffer);
  }

  // Input event parsing.
  for (u32 i = 0; i < arrayLen(game->input.buttons); i++) {
    button_state b = game->input.buttons[i];
    game->input.buttons[i] =
      (b == BUTTON_PRESSED) ? BUTTON_HELD
      : (b == BUTTON_RELEASED) ? BUTTON_UP
      : b;
  }

  for (u32 i = 0; i < arrayLen(game->input.mouse.button); i++) {
    button_state b = game->input.mouse.button[i];
    game->input.mouse.button[i] =
      (b == BUTTON_PRESSED) ? BUTTON_HELD
      : (b == BUTTON_RELEASED) ? BUTTON_UP
      : b;
  }
  game->input.mouse.movement[0] = 0;
  game->input.mouse.movement[1] = 0;

  b32 pausePhysics = game->input.pausePhysics;

  for (u32 i = 0; i < INPUT_BUFFER_SIZE; i++) {
    rt_input_event* input = inputEvents + i;
    if (input->type == RT_INPUT_TYPE_NONE) {
      break;
    }
    if (input->type == RT_INPUT_TYPE_KEY) {
      rt_key_event *keyEvt = &input->key;
      if (strcmp(keyEvt->keycode, "W") == 0) {
	game->input.camMoveUp = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "S") == 0) {
	game->input.camMoveDown = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "A") == 0) {
	game->input.camMoveLeft = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "D") == 0) {
	game->input.camMoveRight = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "Up") == 0) {
	game->input.moveUp = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "Down") == 0) {
	game->input.moveDown = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "Left") == 0) {
	game->input.moveLeft = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "Right") == 0) {
	game->input.moveRight = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "R") == 0) {
	game->input.reset = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "P") == 0 && keyEvt->pressed) {
	game->input.pausePhysics = !game->input.pausePhysics;
      }
      if (strcmp(keyEvt->keycode, "N") == 0 && keyEvt->pressed) {
	pausePhysics = false;
      }
      if (strcmp(keyEvt->keycode, "Escape") == 0) {
	game->input.quit = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "C") == 0 && keyEvt->pressed) {
	game->deve.freeCameraView = !game->deve.freeCameraView;
      }
      if (strcmp(keyEvt->keycode, "F5") == 0 && keyEvt->pressed) {
        game->deve.drawDevePanel = !game->deve.drawDevePanel;
      }
    }
    else if(input->type == RT_INPUT_TYPE_MOUSE) {
      rt_mouse_event *mouse = &input->mouse;
      for (u32 i = 0; i < arrayLen(game->input.mouse.button); i++) {
	if (mouse->buttonIdx) {
	  game->input.mouse.button[mouse->buttonIdx - 1] =
	    setButtonState(mouse->pressed);
	}
	else {

	  game->input.mouse.movement[0] +=
	    (i32)mouse->position[0] - (i32)game->input.mouse.position[0];
	  game->input.mouse.movement[1] +=
	    (i32)mouse->position[1] - (i32)game->input.mouse.position[1];
	  game->input.mouse.position[0] = mouse->position[0];
	  game->input.mouse.position[1] = mouse->position[1];
	  game->input.mouse.wheel = mouse->wheel;
	}
      }
    }
  }
  b32 quit = game->input.quit;

  /// Camera
  vec3s camTarget = game->car.body.chassis.position -
    game->car.body.chassis.localCenter;
  camera_state *cam = &game->camera;
  cam->fov = CLAMP(cam->fov - game->input.mouse.wheel * 5, 45, 90);
  mat4s projM = glms_perspective(glm_rad(cam->fov), float(1280) / float(840),
				 0.1f, 10000.0f);
  mat4s viewM = GLMS_MAT4_IDENTITY_INIT;

  if (game->input.mouse.button[0]) {
    cam->yaw -= game->input.mouse.movement[0] * 0.1f;
    cam->pitch += game->input.mouse.movement[1] * 0.1f;
  }
  /// Free view camera
  if (game->deve.freeCameraView) {
    vec3s forward = {sinf(glm_rad(cam->yaw)) * -sinf(glm_rad(cam->pitch)),
                     -cosf(glm_rad(cam->yaw)), cosf(glm_rad(cam->pitch))};

    vec3s right = {sinf(glm_rad(cam->yaw - 90.f)),
                   -cosf(glm_rad(cam->yaw - 90.f)), 0.0f};

    cam->front = glms_vec3_normalize(forward);
    cam->right = glms_vec3_normalize(right);

    f32 camSpeed = 50.0f * delta;
    if (game->input.camMoveUp >= BUTTON_PRESSED) {
      cam->position = cam->position + cam->front * camSpeed;
    }
    if (game->input.camMoveDown >= BUTTON_PRESSED) {
      cam->position = cam->position - cam->front * camSpeed;
    }
    if (game->input.camMoveLeft >= BUTTON_PRESSED) {
      cam->position = cam->position - cam->right * camSpeed;
    }
    if (game->input.camMoveRight >= BUTTON_PRESSED) {
      cam->position = cam->position + cam->right * camSpeed;
    }

    mat4s pitch = glms_rotate_x(GLMS_MAT4_IDENTITY, glm_rad(cam->pitch));
    mat4s yaw = glms_rotate_z(pitch, -glm_rad(cam->yaw));
    viewM = glms_translate_make(cam->position);
    viewM = yaw * viewM;
  }
  // 3rd person car camera
  else {
    vec3s pos = camTarget - game->car.body.chassis.orientation *
      glms_vec3_rotate((vec3s){5.f,0.f,-3.f}, cam->yaw * 0.1f, GLMS_ZUP);
    cam->position = LERP(cam->position, pos, 0.1f);
    viewM = glms_lookat(cam->position, camTarget, GLMS_ZUP);
  }

  rt_renderer_pushEntry(&rendererBuffer, rt_render_entry_begin);

  rt_render_entry_clear* clearCmd =
    rt_renderer_pushEntry(&rendererBuffer, rt_render_entry_clear);
  clearCmd->clearColor[0] = 90.f/255.f;
  clearCmd->clearColor[1] = 128.f/255.f;
  clearCmd->clearColor[2] = 143.f/255.f;
  clearCmd->clearColor[3] = 1.0f;
  clearCmd->width = 1920;
  clearCmd->height = 1080;

  // Draw functions
  renderSkybox(game, &tempMemory, &rendererBuffer, viewM, projM);
  renderTerrain(game, &tempMemory, &rendererBuffer, viewM, projM);
  renderAndUpdateCar(game,&tempMemory, &rendererBuffer, viewM, projM,
		     pausePhysics ? 0 : delta);

  // Nuklear ui
  if (game->deve.drawDevePanel) {
    ui_draw(ctx, game, delta);
    rt_nuklear_handle_input(inputEvents);
    ctx = rt_nuklear_draw(&tempMemory, &rendererBuffer, platformApi);
  }

  // TODO: Something wonky with counter values, figure it out.
  u64 rendererStartCount = platformApi->getPerformanceCounter();
  platformApi->renderer_flushCommandBuffer(&rendererBuffer);

  // Profiler
  game->profiler.renderingAcc +=
      platformApi->getPerformanceCounter() - rendererStartCount;

  game->profiler.totalAcc +=
      platformApi->getPerformanceCounter() - totalStartCount;

  game->profiler.elapsedTime += delta;
  if (game->profiler.elapsedTime > 1.f) {
    u64 freq = (f64)platformApi->getPerformanceFrequency();
    game->profiler.elapsedTime = game->profiler.elapsedTime - 1.f;
    game->profiler.rendering =
        1000.f * (f64)game->profiler.renderingAcc / 60.f / (f64)freq;

    game->profiler.simulation =
        1000.f * (f64)game->profiler.simulationAcc / 60.f / (f64)freq;

    game->profiler.total =
        1000.f * (f64)game->profiler.totalAcc / 60.f / (f64)freq;

    game->profiler.renderingAcc = 0;
    game->profiler.simulationAcc = 0;
    game->profiler.totalAcc = 0;
  };

  return quit;
}
