#include "all.h"

car_game_state* _game = NULL;

extern "C" RT_GAME_AUDIO_CALLBACK(gameAudioUpdate) {  
  if (_game && _game->initialized) {
    u64 count = platform.api.getPerformanceCounter();
    car_audio_state* carAudioState = &_game->car.audioState;
    i32 rpm = _game->car.stats.rpm;
    f32 invSampleRate = 1.f / audioOut.samplesPerSecond;
    for (i32 i = 0; i < audioOut.sampleCount; i++) {
      i16* bufferOut = (i16*)audioOut.buffer + i * 2;
      f32 audioSample[2] = {0.0f, 0.0f};
      carAudioState->sampleIndex++;
      f32 engineSample = sampleEngineAudio(carAudioState, audioOut.samplesPerSecond, 
                                           rpm, _game->car.stats.accelerating) * 0.3f;

      // Shitty audio effects for sliding and wind
      f32 r = ((f32)rand() / (f32)RAND_MAX) * 2.f - 1.f;
      f32 gravelVolume = MIN(fabs(_game->car.stats.slipAngle[2]),1.0);
      f32 gravelSample = 
        filter(r, invSampleRate, 20,0.2f, Lowpass, carAudioState->gravelFilter);
      gravelSample *= gravelVolume;

      f32 windForce = lerpPerlinNoise1D(carAudioState->sampleIndex * 0.00001f);
      f32 windSample = 
        filter(r, invSampleRate, 500 + 1500 * windForce, 0.8f, Lowpass, carAudioState->windFilter);
      windSample *= windForce * 0.1f;

      // Background music
      i32 ambientIndex = (carAudioState->sampleIndex / 2) % _game->ambient.sampleNum;
      f32 ambientMusicVolume = CLAMP(sinf(carAudioState->sampleIndex * invSampleRate / 15)
                                     + 0.25f, 0.0f, 1.f);
      f32 ambientMusic[2] = {
        (f32)_game->ambient.buffer[ambientIndex * 2] / INT16_MAX * 0.8f * ambientMusicVolume,
        (f32)_game->ambient.buffer[ambientIndex * 2 + 1] / INT16_MAX * 0.8f * ambientMusicVolume
      };

      audioSample[0] = mixAudio(audioSample[0], gravelSample);
      audioSample[1] = mixAudio(audioSample[1], gravelSample);

      audioSample[0] = mixAudio(audioSample[0], engineSample);
      audioSample[1] = mixAudio(audioSample[1], engineSample);

      audioSample[0] = mixAudio(audioSample[0], windSample );
      audioSample[1] = mixAudio(audioSample[1], windSample);

      audioSample[0] = mixAudio(audioSample[0], ambientMusic[0]);
      audioSample[1] = mixAudio(audioSample[1], ambientMusic[1]);

      bufferOut[0] = audioSample[0] * 12000;
      bufferOut[1] = audioSample[1] * 12000;
    }
    count = platform.api.getPerformanceCounter() - count;
    _game->profiler.accumulated[profiler_counter_entry_audio] += count;
  }
}

inline str8 loadBin(const char* path) {
  str8 result;
  result.buffer = (u8*)platformApi->loadBinaryFile(path, &result.len);
  return result;
}

inline rt_shader_data importShader(memory_arena *arena, const char *shaderVsFilePath,
                         const char *shaderFsFilePath) {
  rt_shader_data shaderData = {0};

  shaderData.vsData = loadBin(shaderVsFilePath);
  shaderData.fsData = loadBin(shaderFsFilePath);

  if (shaderData.fsData.len == 0 || shaderData.vsData.len == 0) {
    LOG(LOG_LEVEL_ERROR, "Shader Load failed\n");
  }
  LOG(LOG_LEVEL_DEBUG, "Shader Loaded\n");

  return shaderData;
}


extern "C" RT_GAME_UPDATE_AND_RENDER(gameUpdate) {

  platformApi = &platform.api;
  ASSERT_ = platformApi->assert;
  LOG = platformApi->logger;
  MALLOC = _malloc;
  REALLOC = _realloc;
  FREE = _free;

  memory_arena tempMemory;
  memory_arena permanentMemory;

  memArena_init(&tempMemory, platform.temporaryMemBuffer, platform.temporaryMemSize);
  memArena_init(&permanentMemory, platform.permanentMemBuffer, platform.permanentMemSize);

  stack = &tempMemory;

  car_game_state *game = (car_game_state*)memArena_alloc(&permanentMemory, sizeof(car_game_state));
  _game = game;
  rt_command_buffer rendererBuffer;
  memArena_init(&rendererBuffer.arena,
		memArena_alloc(&tempMemory, KILOBYTES(256)), KILOBYTES(256));
  ui_widget_context* widgetContext = allocUiWidgets(&tempMemory);
  b32 initialize = !game->initialized || reloaded;

  allocTerrain(game, &permanentMemory);
  // Initial component initialization
  if (initialize) {
    LOG(LOG_LEVEL_DEBUG, "terrain loaded");
    if (!game->initialized) {
      game->camera.fov = 90;
      game->camera.position = (v3){-30.0, 25.0, -30.0};
      game->camera.yaw = 0.f;
      game->camera.pitch = 0.f;

      game->debug.visibilityState = visibility_state_car | visibility_state_terrain;
      game->debug.drawDebugPanel = false;
      game->input.pausePhysics = true;
    }
    setupUiWidgets(widgetContext);    
    createSkyBox(game, &permanentMemory, &tempMemory, &rendererBuffer);
    createTerrain(game, &permanentMemory, &tempMemory, &rendererBuffer);
    createCar(game, &permanentMemory, &tempMemory, &rendererBuffer, reloaded,
            assetModTime);
    platformApi->flushCommandBuffer(&rendererBuffer);
    platformApi->flushCommandBuffer(&widgetContext->buffer);
    
    if (!game->initialized) {
    rt_audio_data result = platformApi->loadOGG("assets/soundscape-dust-ambient-quitar.ogg");

     game->ambient = result;
     ASSERT(result.buffer != 0);
    }
    game->initialized = true;
  }

  // Input event parsing.
  for (u32 i = 0; i < arrayLen(game->input.buttons); i++) {
    button_state b = game->input.buttons[i];
    game->input.buttons[i] =
      (b == button_state_pressed) ? button_state_held
      : (b == button_state_released) ? button_state_up
      : b;
  }

  for (u32 i = 0; i < arrayLen(game->input.mouse.button); i++) {
    button_state b = game->input.mouse.button[i];
    game->input.mouse.button[i] =
      (b == button_state_pressed) ? button_state_held
      : (b == button_state_released) ? button_state_up
      : b;
  }
  game->input.mouse.movement[0] = 0;
  game->input.mouse.movement[1] = 0;


  b32 pausePhysics = game->input.pausePhysics;
  for(u32 i = 0; i < input.eventNum; i++) {
    rt_input_event* event = input.events + i;
    if (event->type == rt_input_type_key) {
      rt_key_event *keyEvt = &event->key;
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
        if (game->state != game_state_intro) {
          game->input.pausePhysics = !game->input.pausePhysics;
        }
      }
      if (strcmp(keyEvt->keycode, "Escape") == 0) {
        game->input.quit = setButtonState(keyEvt->pressed);
      }
      if (strcmp(keyEvt->keycode, "Return") == 0 
        && keyEvt->mod & rt_key_mod_left_alt 
        && keyEvt->pressed) {
        platformApi->toggleFullscreen();
      }
      if (strcmp(keyEvt->keycode, "C") == 0 && keyEvt->pressed) {
        if (game->state != game_state_intro) {
          game->debug.freeCameraView = !game->debug.freeCameraView;
        }
      }
      if (strcmp(keyEvt->keycode, "F5") == 0 && keyEvt->pressed) {
        game->debug.drawDebugPanel = !game->debug.drawDebugPanel;
      }
      if (strcmp(keyEvt->keycode, "Space") == 0 && keyEvt->pressed) {
        game->state = game_state_game;
        game->input.pausePhysics = 0;
        game->camera.yaw = 0.f;
        game->camera.pitch = 0.f;
      }
    }
    else if(event->type == rt_input_type_mouse) {
      rt_mouse_event *mouse = &event->mouse;
      game->input.mouse.movement[0] =
        (i32)mouse->delta[0];
      game->input.mouse.movement[1] =
        (i32)mouse->delta[1];
      game->input.mouse.wheel = mouse->wheel;
      game->input.mouse.position[0] = mouse->position[0];
      game->input.mouse.position[1] = mouse->position[1];
    }
    else if(event->type == rt_input_type_mouse_button) {
      rt_mouse_button_event *mouse = &event->mouseButton;
      for (u32 i = 0; i < arrayLen(game->input.mouse.button); i++) {
        game->input.mouse.button[mouse->buttonIdx - 1] =
          setButtonState(mouse->pressed);
      }
    }

  }
  b32 quit = game->input.quit;

  /// Camera
  camera_state *cam = &game->camera;
  cam->fov = CLAMP(cam->fov - game->input.mouse.wheel * 5, 45, 90);
  m4x4 projM = perspective(DEG2RAD(cam->fov), f32(display.size.x) / f32(display.size.y),
				 0.1f, 10000.0f);
  m4x4 viewM = M4X4_IDENTITY;

  /// Free view camera
  m3x3 orientation = game->car.body.chassis.orientation;
  if (game->state == game_state_intro) {
    cam->yaw += 0.1f;
    orientation = m3x3_rotate_make({0.0f, 0.f, -DEG2RAD(cam->yaw)});
    orientation = orientation * m3x3_rotate_make({0.0f, DEG2RAD(cam->pitch),0});
  }
  else if(game->state == game_state_game) {
    if (game->debug.freeCameraView) {
      cam->yaw += game->input.mouse.movement[0] * 2;
      cam->pitch -= game->input.mouse.movement[1] * 2;
      orientation = m3x3_rotate_make({0.0f, 0.f, -DEG2RAD(cam->yaw)});
      orientation = orientation * m3x3_rotate_make({0.0f, DEG2RAD(cam->pitch),0});
    }
  }
  v3 camTarget = game->car.body.chassis.position -
    game->car.body.chassis.localCenter;
  v3 camOffset = (v3){5.f,0.f,-3.f} * orientation;
  v3 camPos = camTarget - camOffset;
  cam->position = LERP(cam->position, camPos, 
                       (game->debug.freeCameraView || initialize) ? 1.0f : 0.1f);
  viewM = lookAt(cam->position - camTarget,(v3){0.0f,0.0f, 1.f});

  rt_pushRenderCommand(&rendererBuffer, begin);

  rt_command_clear* clearCmd =
    rt_pushRenderCommand(&rendererBuffer, clear);
  clearCmd->clearColor[0] = 90.f/255.f;
  clearCmd->clearColor[1] = 128.f/255.f;
  clearCmd->clearColor[2] = 143.f/255.f;
  clearCmd->clearColor[3] = 1.0f;
  clearCmd->width = display.size.x;
  clearCmd->height = display.size.y;

   // Draw functions
  renderSkybox(game, &tempMemory, &rendererBuffer, viewM, projM);
  renderTerrain(game, &tempMemory, &rendererBuffer, viewM, projM);
  updateCar(game,&tempMemory, &rendererBuffer,
                     widgetContext, viewM, projM,
		     pausePhysics ? 0 : time.delta);

  renderCar(game, &tempMemory, &rendererBuffer, widgetContext, viewM, projM);
  // Nuklear ui
  readyUiWidgets(widgetContext, display.size);
  profilerBegin(game->profiler, ui);
  drawUI(widgetContext,
          game, display.size,
          time.delta, &tempMemory);
  profilerEnd(game->profiler, ui);
  renderUIWidgets(widgetContext, &tempMemory);

  profilerBegin(game->profiler, rendering);
  platformApi->flushCommandBuffer(&rendererBuffer);
  platformApi->flushCommandBuffer(&widgetContext->buffer);

  // Profiler
  profilerEnd(game->profiler, rendering);
  profilerAverage(game->profiler, time.delta, 1.0);
  return quit;
}

