#define SDL_MAIN_HANDLED

#ifndef DYNAMIC_LIB_LOAD
#define DYNAMIC_LIB_LOAD 1
#endif

#include "SDL.h"
#include "SDL_keyboard.h"
#include "SDL_loadso.h"
#include "SDL_log.h"
#include "SDL_stdinc.h"

#include "SDL_video.h"
#include "core/types.h"
#include "core/core.h"
#include "core/mem.h"
#include "core/rotten_renderer.h"
#include "rotten_platform.h"

static void LOG(LogLevel logLevel, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  SDL_LogPriority prio;
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

#define ASSERT(cond) ASSERT_(cond, #cond, SDL_FUNCTION, SDL_LINE, SDL_FILE)

#include "core/mem_arena.c"
#include "core/mem_free_list.c"
#include "core/file.c"
#include "core/sdl_file.c"

static SDL_Window* sdlWindow;
static SDL_GLContext glContext;

static u32 windowWidth;
static u32 windowHeight;

typedef struct game_code {
  void* libGameCode;
  utime libLastWriteTime;

  rt_gameUpdate* gameLoopFunc;

  b32 isValid;
} game_code;

typedef struct renderer_code {
  void* libRendererCode;
  utime libLastWriteTime;

  rt_renderer_flushCommandBuffer* flushCommandFunc;
  rt_renderer_init* initRendererFunc;


  b32 isValid;
} renderer_code;


static b32 SDLLoadGameCode(const char *libName, game_code *code, utime wt) {
  LOG(LOG_LEVEL_DEBUG, "Load Game code\n");
  code->libLastWriteTime = wt;

  code->libGameCode = SDL_LoadObject(libName);
  if (code->libGameCode) {
    code->gameLoopFunc = (rt_gameUpdate *)SDL_LoadFunction(
        code->libGameCode, "gameUpdate");

    code->isValid = code->gameLoopFunc != 0;
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

static void createWindow(u32 width, u32 height) {

  windowWidth = width;
  windowHeight = height;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

  sdlWindow = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, width, height,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  ASSERT(sdlWindow != NULL);
  SDL_GL_SetSwapInterval(1);
  glContext = SDL_GL_CreateContext(sdlWindow);

  if (!glContext) {
    LOG(LOG_LEVEL_ERROR, "GL Context creation failed. SDL_Error: %s\n",
	SDL_GetError());
    ASSERT(false);
  }
}

static void swapWindow() {
  SDL_GL_SwapWindow(sdlWindow);
}

static void deleteContext() {
  SDL_GL_DeleteContext(glContext);
}

static void *diskIOReadFile(const char *filePath, u32 *size, b32 isBinary) {
  void *data =
      (isBinary) ? SDLReadFileB(filePath, size) : SDLReadFile(filePath, size);
  return data;
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

static void parseInputs(rt_input_event *buffer) {
  u32 inputIdx = 0;
  for (SDL_Event event; SDL_PollEvent(&event) != 0;) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      rt_input_event *input = buffer + inputIdx++;
      input->type = RT_INPUT_TYPE_KEY;
      ASSERT(inputIdx < INPUT_BUFFER_SIZE);
      input->key.pressed = event.type == SDL_KEYDOWN;
      const char* keyStr = SDL_GetKeyName(event.key.keysym.sym);
      usize keyStrLen =  arrayLen(((rt_key_event*)0)->keycode);
      ASSERT(strlen(keyStr) < keyStrLen);
      SDL_memcpy(input->key.keycode, keyStr, keyStrLen);
    }

    if (event.type == SDL_MOUSEMOTION ) {
      rt_input_event *input = buffer + inputIdx++;
      input->type = RT_INPUT_TYPE_MOUSE;
      ASSERT(inputIdx < INPUT_BUFFER_SIZE);
      input->mouse.position[0] = event.motion.x;
      input->mouse.position[1] = event.motion.y;
    }

    if (event.type == SDL_MOUSEWHEEL) {
      rt_input_event *input = buffer + inputIdx++;
      input->type = RT_INPUT_TYPE_MOUSE;
      ASSERT(inputIdx < INPUT_BUFFER_SIZE);
      input->mouse.wheel = event.wheel.y;
      input->mouse.position[0] = event.motion.x;
      input->mouse.position[1] = event.motion.y;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      rt_input_event *input = buffer + inputIdx++;
      input->type = RT_INPUT_TYPE_MOUSE;
      ASSERT(inputIdx < INPUT_BUFFER_SIZE);

      input->mouse.buttonIdx = event.button.button;
      input->mouse.pressed = event.type == SDL_MOUSEBUTTONDOWN;
      input->mouse.position[0] = event.motion.x;
      input->mouse.position[1] = event.motion.y;
    }
  if (inputIdx >= INPUT_BUFFER_SIZE - 1) {
    break;
  }
  }
}

int main(int arc, char **argv) {
  // Reserve memory
  platform_state platform = {};
#if DYNAMIC_LIB_LOAD
  ASSERT(arc == 3 && "Please give game and renderer library names");
  const char *gameCodeLib = argv[1];
  const char *rendererCodeLib = argv[2];
#endif
  u32 permanentMemSize = MEGABYTES(32);
  u32 temporaryMemSize = MEGABYTES(512);

  u64 mainMemorySize = permanentMemSize + temporaryMemSize;

  void *mainMemoryBuffer = SDL_malloc(mainMemorySize);
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    LOG(LOG_LEVEL_ERROR, "SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
    return 0;
  }

  createWindow(1920, 1080);

  platform.permanentMemBuffer = mainMemoryBuffer;
  platform.transientMemBuffer = mainMemoryBuffer +
		+ permanentMemSize;

  platform.permanentMemSize = permanentMemSize;
  platform.transientMemSize = temporaryMemSize;

  // Api bindings
  platform.api.logger = LOG;
  platform.api.assert = ASSERT_;
  platform.api.diskIOReadFile = diskIOReadFile;
  platform.api.diskIOReadFileTo = diskIOReadFileTo;
  platform.api.diskIOReadFileSize = diskIOReadFileSize;
  platform.api.diskIOReadModTime = fileReadModTime;
  platform.api.getPerformanceCounter = SDL_GetPerformanceCounter;
  platform.api.getPerformanceFrequency = SDL_GetPerformanceFrequency;

  game_code gameCode;
  renderer_code rendererCode;

  gameCode.libGameCode = NULL;
  rendererCode.libRendererCode = NULL;

#if DYNAMIC_LIB_LOAD
  SDLLoadGameCode(gameCodeLib, &gameCode, fileReadModTime(gameCodeLib));
  SDLLoadRendererCode(rendererCodeLib, &rendererCode, fileReadModTime(rendererCodeLib));
  platform.api.renderer_flushCommandBuffer = rendererCode.flushCommandFunc;
#else
  platform.api.renderer_flushCommandBuffer = flushCommandBuffer;
  gameCode.gameLoopFunc = gameUpdate;
  rendererCode.initRendererFunc = rendererInit;
#endif

  rendererCode.initRendererFunc(LOG, ASSERT_, SDL_GL_GetProcAddress);
  rt_input_event inputEventBuffer[INPUT_BUFFER_SIZE];

  b32 quit = false;

  u32 t = 0.0;
  u32 targetTimeMSec = 1000 / 60;

  u32 currentTime = SDL_GetTicks();
  u32 elapsedTimeMSec = 0;
  u32 hotReloadTime = 0;
  utime assetFileModTime = fileReadModTime("assets/modfile");
  b32 initialRun = true;
  while (!quit) {
    b32 reloaded = false;
#if DYNAMIC_LIB_LOAD
    utime newAssetFileModTime = assetFileModTime;
    if (hotReloadTime > 1000) {
      newAssetFileModTime = fileReadModTime("assets/modfile");
      reloaded = newAssetFileModTime > assetFileModTime;
      if (!SDLFileExists("./librottengame.lock")) {
        utime wt = fileReadModTime(gameCodeLib);
        if (wt != 0 && wt > gameCode.libLastWriteTime) {
          SDL_UnloadObject(gameCode.libGameCode);
          SDLLoadGameCode(gameCodeLib, &gameCode, wt);
          gameCode.gameLoopFunc = (rt_gameUpdate *)SDL_LoadFunction(
              gameCode.libGameCode, "gameUpdate");
          reloaded = true;
        }
      }
      if (!SDLFileExists("./librenderer.lock")) {
        utime wt = fileReadModTime(rendererCodeLib);
        if (wt != 0 && wt > rendererCode.libLastWriteTime) {
          SDL_UnloadObject(rendererCode.libRendererCode);
          SDLLoadRendererCode(rendererCodeLib, &rendererCode, wt);
          platform.api.renderer_flushCommandBuffer =
              rendererCode.flushCommandFunc;
        }
      }
      hotReloadTime = 0;
      assetFileModTime = newAssetFileModTime;
    }
#endif
    u32 newTime = SDL_GetTicks();
    u32 frameTimeMS = newTime - currentTime;
    currentTime = newTime;

    t += frameTimeMS;

    if (frameTimeMS > targetTimeMSec) {
      frameTimeMS = targetTimeMSec;
    }

    elapsedTimeMSec += frameTimeMS;
    hotReloadTime += frameTimeMS;
    while (elapsedTimeMSec >= targetTimeMSec) {
      elapsedTimeMSec -= targetTimeMSec;
      float dt = (float)targetTimeMSec / 1000.f;
      SDL_memset(inputEventBuffer, 0, sizeof(inputEventBuffer));
      parseInputs(inputEventBuffer);
      quit = gameCode.gameLoopFunc(dt, t, &platform, inputEventBuffer,
                                   reloaded, initialRun ? 0 : assetFileModTime);
      swapWindow();
      initialRun = false;
    }
  }
  deleteContext();

  SDL_Quit();

  return 0;
}
