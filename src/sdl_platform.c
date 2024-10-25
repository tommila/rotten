#include "SDL_video.h"
#define SDL_MAIN_HANDLED

#ifndef DYNAMIC_LIB_LOAD
#define DYNAMIC_LIB_LOAD 1
#endif

// Set this to 1 use old audio ring buffer.
// SDL implements it's own double-buffered audio so
// extra ring buffer is not needed.
#define AUDIO_RING_BUFFER 0

#define ASSERT(cond) ASSERT_(cond, #cond, SDL_FUNCTION, SDL_LINE, SDL_FILE)

#include "SDL.h"

#include <string.h>
#include "core/types.h"
#include "core/core.h"
#include "core/mem.h"
#include "core/math.h"
#include "core/string.h"
#include "core/rotten_renderer.h"
#include "rotten_platform.h"

static void LOG(LogLevel logLevel, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  SDL_LogPriority prio = SDL_LOG_PRIORITY_ERROR;
  switch (logLevel) {
    case LOG_LEVEL_VERBOSE:
      prio = SDL_LOG_PRIORITY_VERBOSE;
      break;
    case LOG_LEVEL_DEBUG:
      prio = SDL_LOG_PRIORITY_DEBUG;
      break;
    case LOG_LEVEL_WARN:
      prio = SDL_LOG_PRIORITY_WARN;
      break;
    case LOG_LEVEL_ERROR:
      prio = SDL_LOG_PRIORITY_ERROR;
      break;
  }
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, prio, format, ap);
  va_end(ap);
}
static void ASSERT_(b32 cond, const char *condText, const char *function,
                    i32 linenum, const char *filename) {
  do {
    while (!(cond)) {
      struct SDL_AssertData sdl_assert_data = {0, 0, condText, 0, 0, 0, 0};
      const SDL_AssertState sdl_assert_state =
          SDL_ReportAssertion(&sdl_assert_data, function, filename, linenum);
      if (sdl_assert_state == SDL_ASSERTION_RETRY) {
        continue; /* go again. */
      } else if (sdl_assert_state == SDL_ASSERTION_BREAK) {
        SDL_TriggerBreakpoint();
      }
      break; /* not retrying. */
    }
  } while (SDL_NULL_WHILE_LOOP_CONDITION);
}


memory_arena SDLMemArena, platformMemArena;

// TODO: Implement free list sdl memory allocation
//static void* _SDL_malloc(usize s) {
//  return memArena_alloc(&SDLMemArena, s);
//}
//static void* _SDL_calloc(usize membNum, usize s) {
//  return memArena_calloc(&SDLMemArena, membNum, s);
//}
//static void* _SDL_realloc(void* ptr, usize s) {
//  return memArena_realloc(&SDLMemArena, ptr, s);
//}
//static void _SDL_free(void* ptr) {}

#include "core/mem_arena.c"
#include "core/mem_free_list.c"
#include "core/file.c"
#include "core/sdl_file.c"
#include "core/image.c"
#include "core/audio.c"

static SDL_Window* sdlWindow;
static SDL_GLContext glContext;

static u32 windowWidth;
static u32 windowHeight;

typedef struct game_code {
  void* libGameCode;
  utime libLastWriteTime;

  rt_gameUpdate* gameLoopFunc;
  rt_gameAudioUpdate* gameAudioFunc;

  b32 isValid;
} game_code;

typedef struct renderer_code {
  void* libRendererCode;
  utime libLastWriteTime;

  rt_flushCommandBuffer* flushCommandFunc;
  rt_init* initRendererFunc;

  b32 isValid;
} renderer_code;

typedef struct audio_callback_data {
  platform_state* platform;
#if AUDIO_RING_BUFFER
  audio_buffer* out;
#else
  audio_out* out;
  rt_gameAudioUpdate* gameAudioFunc;
#endif
} audio_callback_data;

static b32 SDLLoadGameCode(const char *libName, game_code *code, utime wt) {
  LOG(LOG_LEVEL_DEBUG, "Load Game code\n");
  code->libLastWriteTime = wt;

  code->libGameCode = SDL_LoadObject(libName);
  if (code->libGameCode) {
    code->gameLoopFunc = (rt_gameUpdate *)SDL_LoadFunction(
        code->libGameCode, "gameUpdate");

    code->gameAudioFunc = (rt_gameAudioUpdate *)SDL_LoadFunction(
        code->libGameCode, "gameAudioUpdate");

    code->isValid = (code->gameLoopFunc && code->gameAudioFunc) != 0;
  }
  if (!code->libGameCode) {
    LOG(LOG_LEVEL_ERROR, "Could not load game code: %s\n", SDL_GetError());
    ASSERT(false);
  }

  return code != 0;
}

static b32 SDLLoadRendererCode(const char *libName, renderer_code *code, utime wt) {
  LOG(LOG_LEVEL_DEBUG, "Load Game code\n");
  code->libLastWriteTime = wt;

  code->libRendererCode = SDL_LoadObject(libName);
  if (code->libRendererCode) {
    code->flushCommandFunc = SDL_LoadFunction(
        code->libRendererCode, "flushCommandBuffer");
    code->initRendererFunc = SDL_LoadFunction(
        code->libRendererCode, "rendererInit");

    code->isValid = code->flushCommandFunc != 0;
  }
  if (!code->libRendererCode) {
    LOG(LOG_LEVEL_ERROR, "Could not load game code: %s\n", SDL_GetError());
    ASSERT(false);
  }

  return code != 0;
}

static void createWindow() {

  SDL_Rect bounds;
  SDL_GetDisplayBounds(0, &bounds);
  i32 displayModeNum = SDL_GetNumDisplayModes(0);
  LOG(LOG_LEVEL_DEBUG, "Checking display modes");
  for (i32 i = 0; i < displayModeNum; i++) {
    SDL_DisplayMode mode = {0};
    SDL_GetDisplayMode(0, i, &mode);
    LOG(LOG_LEVEL_DEBUG, "%d: %dx%d %d Hz", i, mode.w, mode.h, mode.refresh_rate);
  }

  SDL_DisplayMode mode = {0};
  SDL_GetDisplayMode(0, 0, &mode);
  windowWidth = mode.w;
  windowHeight = mode.h;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

  sdlWindow = SDL_CreateWindow("Car 'n' sand", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
if (!sdlWindow) {
    LOG(LOG_LEVEL_ERROR, "SDL window creation failed. SDL_Error: %s\n",
	SDL_GetError());
    ASSERT(false);
  }
  SDL_GL_SetSwapInterval(1);
  glContext = SDL_GL_CreateContext(sdlWindow);

  if (!glContext) {
    LOG(LOG_LEVEL_ERROR, "GL Context creation failed. SDL_Error: %s\n",
	SDL_GetError());
    ASSERT(false);
  }
}

static void toggleFullscreen() {
  u32 mode = (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP) 
    ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;
  SDL_SetWindowFullscreen(sdlWindow, mode);
}

static void swapWindow() {
  SDL_GL_SwapWindow(sdlWindow);
}

static void deleteContext() {
  SDL_DestroyWindow(sdlWindow);
  SDL_GL_DeleteContext(glContext);
}

static void diskIOReadFileTo(const char *filePath, u32 dataSize,
                             b32 isBinary, void *dataOut) {
  (isBinary) ? SDLReadFileBTo(filePath, dataSize, dataOut)
             : SDLReadFileTo(filePath, dataSize, dataOut);
}

static u32 diskIOReadFileSize(const char *filePath, b32 isBinary) {
  u32 size =
      (isBinary) ? SDLReadFileSizeB(filePath) : SDLReadFileSize(filePath);
  return size;
}

static void* _loadBinaryFile(const char* path, usize* fileSizeOut) {
  usize fileSize = diskIOReadFileSize(path, true);
  void* fileBuffer = 0;
  if (fileSize > 0) {
    fileBuffer = (void*)pushSize(&platformMemArena, fileSize);
    diskIOReadFileTo(path, fileSize, true, fileBuffer);
    *fileSizeOut = fileSize; 
  }
  return fileBuffer;
}

static char* _loadTextFile(const char* path, usize* fileSizeOut) {
  usize fileSize = diskIOReadFileSize(path, false);
  char* fileBuffer = 0;
  if (fileSize > 0) {
    fileBuffer = (void*)pushSize(&platformMemArena, fileSize);
    diskIOReadFileTo(path, fileSize, false, fileBuffer);
    *fileSizeOut = fileSize; 
  }
  return fileBuffer;
}

static rt_audio_data _loadOGG(const char *path) {
  rt_audio_data result = loadOGG(path);
  return result;
}

static rt_image_data _loadImage(const char *path, u8 channelNum) {
  rt_image_data result = loadImage(path, channelNum);
  return result;
}

static void parseInputs(rt_input* input) {
  input->eventNum = 0;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      rt_input_event *inputEvent = input->events + input->eventNum++;

      inputEvent->type = rt_input_type_key;
      ASSERT(input->eventNum < INPUT_BUFFER_SIZE);
      inputEvent->key.pressed = event.type == SDL_KEYDOWN;
      const char* keyStr = SDL_GetKeyName(event.key.keysym.sym);
      usize keyStrLen =  arrayLen(((rt_key_event*)0)->keycode);
      ASSERT(strlen(keyStr) < keyStrLen);
      SDL_memcpy(inputEvent->key.keycode, keyStr, keyStrLen);

      inputEvent->key.mod |= (event.key.keysym.mod) & KMOD_LALT ? rt_key_mod_left_alt : 0;
      inputEvent->key.mod |= (event.key.keysym.mod) & KMOD_LSHIFT ? rt_key_mod_left_shift : 0;
    }

    if (event.type == SDL_MOUSEMOTION ) {
      rt_input_event *inputEvent = input->events + input->eventNum++;
      inputEvent->type = rt_input_type_mouse;
      ASSERT(input->eventNum < INPUT_BUFFER_SIZE);
      inputEvent->mouse.position[0] = event.motion.x;
      inputEvent->mouse.position[1] = event.motion.y;
      inputEvent->mouse.delta[0] = event.motion.xrel;
      inputEvent->mouse.delta[1] = event.motion.yrel;
    }

    if (event.type == SDL_MOUSEWHEEL) {
      rt_input_event *inputEvent = input->events + input->eventNum++;
      inputEvent->type = rt_input_type_mouse;
      ASSERT(input->eventNum < INPUT_BUFFER_SIZE);
      inputEvent->mouse.wheel = event.wheel.y;
      inputEvent->mouse.position[0] = event.motion.x;
      inputEvent->mouse.position[1] = event.motion.y;
      inputEvent->mouse.delta[0] = event.motion.xrel;
      inputEvent->mouse.delta[1] = event.motion.yrel;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      rt_input_event *inputEvent = input->events + input->eventNum++;
      inputEvent->type = rt_input_type_mouse_button;
      ASSERT(input->eventNum < INPUT_BUFFER_SIZE);

      inputEvent->mouseButton.buttonIdx = event.button.button;
      inputEvent->mouseButton.pressed = event.type == SDL_MOUSEBUTTONDOWN;
    }
    if (input->eventNum >= INPUT_BUFFER_SIZE - 1) {
      break;
    }
  }
}

static void SDLAudioCallback(void *userdata, u8 *deviceBuffer, i32 bufferLength) {
  audio_callback_data* cb = (audio_callback_data*)userdata;
#if AUDIO_RING_BUFFER
  audio_buffer* buff = cb->out;
  i32 regionSize1 = bufferLength;
  i32 regionSize2 = 0;
  if ((buff->readCursor + bufferLength) > buff->size) {
    regionSize1 = buff->size - buff->readCursor;
    regionSize2 = bufferLength - regionSize1;
  }
  SDL_memcpy(deviceBuffer, (buff->buffer + buff->readCursor), regionSize1);
  SDL_memcpy(deviceBuffer + regionSize1, buff->buffer, regionSize2);

  buff->readCursor = (buff->readCursor + bufferLength) % buff->size;
  buff->bytesToBeWritten += bufferLength;
#else
  audio_out out = *cb->out;
  out.sampleCount = bufferLength / out.bytesPerSample;
  out.size = bufferLength;
  out.buffer = deviceBuffer;
  cb->gameAudioFunc(out, *cb->platform);
#endif
}

static void SDLFillAudioBuffer(audio_buffer* buffer, audio_out* sampleData) {
  i32 regionSize1 = sampleData->size; // MIN(sampleData->size, sampleData->size);
  i32 regionSize2 = 0;
  if ((buffer->writeCursor + sampleData->size) > buffer->size) {
    regionSize1 = buffer->size - buffer->writeCursor;
    regionSize2 = sampleData->size - regionSize1;
    ASSERT(regionSize1 > 0 && regionSize2 > 0);
  }

  SDL_memcpy((buffer->buffer + buffer->writeCursor), sampleData->buffer, regionSize1);
  SDL_memcpy(buffer->buffer, (sampleData->buffer + regionSize1), regionSize2);
}

int main(int arc, char **argv) {
  // Reserve memory
  platform_state platform = {};
  game_code gameCode = {};
  renderer_code rendererCode = {};

#if DYNAMIC_LIB_LOAD
  ASSERT(arc == 3 && "Please give game and renderer library names");
  const char *gameCodeLib = argv[1];
  const char *rendererCodeLib = argv[2];
#endif

  u32 SDLMemSize           = KILOBYTES(512);
  u32 platformMemSize      = MEGABYTES(128);

  u32 gamePermanentMemSize = MEGABYTES(32);
  u32 gameTemporaryMemSize = MEGABYTES(64);
  
  u64 totalMemorySize = SDLMemSize + 
    platformMemSize + 
    gamePermanentMemSize +
    gameTemporaryMemSize; 
  
  void *memoryBuffer = SDL_calloc(1, totalMemorySize);
    
  memArena_init(&SDLMemArena, memoryBuffer, SDLMemSize);
  memArena_init(&platformMemArena, memoryBuffer + SDLMemSize, platformMemSize);

  memoryBuffer = memoryBuffer + SDLMemSize + platformMemSize;

  platform.permanentMemBuffer = memoryBuffer;
  platform.temporaryMemBuffer = memoryBuffer + gamePermanentMemSize;

  platform.permanentMemSize = gamePermanentMemSize;
  platform.temporaryMemSize = gameTemporaryMemSize;

  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) < 0) {
    LOG(LOG_LEVEL_ERROR, "SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
    return 0;
  }

  createWindow();
  //SDL_SetRelativeMouseMode(true);
  // Api bindings
  platform.api.logger = LOG;
  platform.api.assert = ASSERT_;
  platform.api.toggleFullscreen = toggleFullscreen;
  platform.api.readFileModTime = readFileModTime;
  platform.api.loadOGG = _loadOGG;
  platform.api.loadImage = _loadImage;
  platform.api.loadBinaryFile = _loadBinaryFile;
  platform.api.loadTextFile = _loadTextFile;
  platform.api.getPerformanceCounter = SDL_GetPerformanceCounter;
  platform.api.getPerformanceFrequency = SDL_GetPerformanceFrequency;

  gameCode.libGameCode = NULL;
  rendererCode.libRendererCode = NULL;

#if DYNAMIC_LIB_LOAD
  SDLLoadGameCode(gameCodeLib, &gameCode, readFileModTime(gameCodeLib));
  SDLLoadRendererCode(rendererCodeLib, &rendererCode, readFileModTime(rendererCodeLib));
  platform.api.flushCommandBuffer = rendererCode.flushCommandFunc;
#else
  platform.api.flushCommandBuffer = flushCommandBuffer;
  gameCode.gameLoopFunc = gameUpdate;
  rendererCode.initRendererFunc = rendererInit;
#endif

  SDL_AudioSpec audioSpecIn, audioSpec;
  audio_buffer audioBuffer = {0};
  audio_out audioOut = {0};
  audio_callback_data audioCallbackData = {
    .platform = &platform,
#if AUDIO_RING_BUFFER    
    .out = &audioBuffer,
#else
    .out = &audioOut,
    .gameAudioFunc = gameCode.gameAudioFunc
#endif
  };
  SDL_memset(&audioSpecIn, 0, sizeof(audioSpecIn));

  audioSpecIn.freq     = 44100;
  audioSpecIn.format   = AUDIO_S16SYS;
  audioSpecIn.channels = 2;
  audioSpecIn.samples  = 2048;
  audioSpecIn.callback = &SDLAudioCallback;
  audioSpecIn.userdata = &audioCallbackData;
  SDL_AudioDeviceID audioDeviceId = 0;
  audioDeviceId = SDL_OpenAudioDevice(
    NULL, 0,
    &audioSpecIn, &audioSpec, 0
  );
  
  if(!audioDeviceId)
  {
    LOG(LOG_LEVEL_ERROR, "Error creating SDL audio device. SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }
  i32 bytesPerSample = audioSpec.channels * sizeof(i16);
  // 16ms latencyt
  i32 latencySampleCount = audioSpec.freq / 60;
  i32 latencySize = latencySampleCount * bytesPerSample;
  // 5 sec buffer size
  audioBuffer.size = audioSpec.freq * 5 * bytesPerSample;
  audioBuffer.buffer = pushSize(&platformMemArena, audioBuffer.size);
  audioBuffer.bytesPerSample = bytesPerSample;
  audioBuffer.bytesToBeWritten = 0;
  audioBuffer.readCursor = 0;
  audioBuffer.writeCursor = latencySize;
  audioBuffer.sampleIndex = 0;
  SDL_memset(audioBuffer.buffer, 0, audioBuffer.size);  

  audioOut.samplesPerSecond = audioSpec.freq;
  audioOut.bytesPerSample = bytesPerSample;
#if AUDIO_RING_BUFFER
  audioOut.sampleCount = audioSpec.samples;
  audioOut.size = audioSpec.samples * bytesPerSample;
  audioOut.buffer = pushSize(&platformMemArena, audioSpec.samples * bytesPerSample);
  SDL_memset(audioOut.buffer, 0, audioOut.size);
  
  SDL_QueueAudio(audioDeviceId, audioOut.buffer, audioOut.size);
#endif
  SDL_PauseAudioDevice(audioDeviceId, 0);

  // No memory system allocation after this point
  //SDL_SetMemoryFunctions(_SDL_malloc, _SDL_calloc, _SDL_realloc, _SDL_free);
  //i32 *p = SDL_malloc(10);
  
  rendererCode.initRendererFunc(LOG, ASSERT_, SDL_GL_GetProcAddress);

  b32 quit = false;

  u32 duration = 0.0;
  u32 targetTimeMSec = 1000 / 60;

  u32 currentTime = SDL_GetTicks();
  u32 elapsedTimeMSec = 0;
  u32 hotReloadTime = 0;
  utime assetFileModTime = readFileModTime("assets/modfile");
  b32 initialRun = true;
  rt_input input = {
    pushArray(&platformMemArena, INPUT_BUFFER_SIZE, rt_input_event),
    0
  };
  while (!quit) {
    memArena_clear(&SDLMemArena);
    b32 reloaded = false;
#if DYNAMIC_LIB_LOAD
    utime newAssetFileModTime = assetFileModTime;
    if (hotReloadTime > 1000) {
      newAssetFileModTime = readFileModTime("assets/modfile");
      reloaded = newAssetFileModTime > assetFileModTime;
      if (!SDLFileExists("./readlock")) {
        utime wt = readFileModTime(gameCodeLib);
        if (wt != 0 && wt > gameCode.libLastWriteTime) {
          SDL_LockAudioDevice(audioDeviceId);
          SDL_UnloadObject(gameCode.libGameCode);
          SDLLoadGameCode(gameCodeLib, &gameCode, wt);
          gameCode.gameLoopFunc = (rt_gameUpdate *)SDL_LoadFunction(
              gameCode.libGameCode, "gameUpdate");
          gameCode.gameAudioFunc = (rt_gameAudioUpdate *)SDL_LoadFunction(
              gameCode.libGameCode, "gameAudioUpdate");
          audioCallbackData.gameAudioFunc = gameCode.gameAudioFunc;
          reloaded = true;
          SDL_UnlockAudioDevice(audioDeviceId);
        }
      }
     // Renderer hotreload not working, disable it for now
     // if (!SDLFileExists("./librenderer.lock")) {
     //   utime wt = readFileModTime(rendererCodeLib);
     //   if (wt != 0 && wt > rendererCode.libLastWriteTime) {
     //     SDL_UnloadObject(rendererCode.libRendererCode);
     //     SDLLoadRendererCode(rendererCodeLib, &rendererCode, wt);
     //     platform.api.renderer_flushCommandBuffer =
     //         rendererCode.flushCommandFunc;
     //   }
     // }
      hotReloadTime = 0;
      assetFileModTime = newAssetFileModTime;
    }
#endif
    u32 newTime = SDL_GetTicks();
    u32 frameTimeMS = newTime - currentTime;
    currentTime = newTime;

    duration += frameTimeMS;

    if (frameTimeMS > targetTimeMSec) {
      frameTimeMS = targetTimeMSec;
    }

    elapsedTimeMSec += frameTimeMS;
    hotReloadTime += frameTimeMS;

    while (elapsedTimeMSec >= targetTimeMSec) {
      elapsedTimeMSec -= targetTimeMSec;
      f32 dt = (f32)targetTimeMSec / 1000.f;

      parseInputs(&input);
      quit = gameCode.gameLoopFunc(
        (frame_time){.delta = dt, .duration = duration},
        (display_state){.size = {windowWidth, windowHeight}},
        platform, input,
        reloaded, initialRun ? 0 : assetFileModTime);
      swapWindow();
      initialRun = false;
    }
#if AUDIO_RING_BUFFER
    i32 readCursor = audioBuffer.readCursor;
    i32 audioBytes = audioBuffer.bytesToBeWritten;
    i32 audioByteSamples = audioBytes / bytesPerSample;
    if (audioDeviceId && audioBytes > 0) {
      i32 updateNum = audioByteSamples / audioSpec.samples;
      for (i32 i = 0; i < updateNum; i++) {
        audioBuffer.writeCursor = (readCursor + latencySize) % audioBuffer.size;
        SDL_memset(audioOut.buffer, 0, audioOut.size);
        gameCode.gameAudioFunc(platform, audioOut);
        SDL_LockAudioDevice(audioDeviceId);
        SDLFillAudioBuffer(&audioBuffer, &audioOut);
        audioBuffer.bytesToBeWritten -= audioSpec.samples * bytesPerSample;
        SDL_UnlockAudioDevice(audioDeviceId);
      }
    }
#endif
  }
  deleteContext();  
  SDL_PauseAudioDevice(audioDeviceId, 1);
  SDL_CloseAudioDevice(audioDeviceId);
  SDL_Quit();

  return 0;
}
