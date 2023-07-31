#include "platform.h"

#include <sys/mman.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "SDL2/SDL.h"
#include "SDL2/SDL_assert.h"
#include "SDL2/SDL_platform.h"
#include "SDL2/SDL_syswm.h"
#include "bgfx/c99/bgfx.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include "core/bgfx_renderer.cpp"
#include "core/file.cpp"
#include "core/input.cpp"
#include "core/sdl_file.cpp"
#include "core/sdl_lib_loader.cpp"
#include "ext/cJSON.h"

// TODO: move to own spesific file
static void initDisplay(uint16_t width, uint16_t height) {
  // Init SDL window
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_Window *sdlWindow =
      SDL_CreateWindow("tst", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       width, height, SDL_WINDOW_SHOWN);

  SDL_ASSERT(sdlWindow);

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  SDL_ASSERT(SDL_GetWindowWMInfo(sdlWindow, &wmi));

  void *window = NULL;
  void *display = NULL;
#if __WINDOWS__
  window = wmi.info.win.window;
#elif __MACOSX__
#elif __LINUX__ || __NETBSD__ || __OPENBSD__
  display = (void *)(uintptr_t)wmi.info.x11.display;
  window = (void *)(uintptr_t)wmi.info.x11.window;
#elif __ANDROID__
  window = (void *)"#canvas";
#else
  printf("unknown platform\n");
  SDL_ASSERT(false);
#endif  // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
        // BX_PLATFORM_EMSCRIPTEN

  SDL_ASSERT(rendererInit(width, height, window, display));
  SDL_ASSERT(imGuiInitSDL(sdlWindow));
  imGuiInitBgfx();
}

static void *diskIOReadFile(const char *filePath, size_t *size, bool isBinary) {
  void *data =
      (isBinary) ? SDLReadFileB(filePath, size) : SDLReadFile(filePath, size);
  return data;
}

int main(int arc, char **argv) {
  size_t permanentMemSize = MEGABYTES(64);
  void *permanentMem = calloc(1, permanentMemSize);

  // Init SDL2 subsystems and create window
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
    return 0;
  }

  // Reserve memory
  platform_state platform = {};

  platform.permanentMemory.size = MEGABYTES(64);
  platform.temporaryMemory.size = MEGABYTES(64);

  void *baseAddress = 0;

  uint64_t gameMemorySize =
      platform.permanentMemory.size + platform.temporaryMemory.size;
  void *gameMemoryBlock =
      mmap(baseAddress, (size_t)gameMemorySize, PROT_READ | PROT_WRITE,
           MAP_ANON | MAP_PRIVATE, -1, 0);

  platform.permanentMemory.data = gameMemoryBlock;
  platform.temporaryMemory.data = ((uint8_t *)platform.permanentMemory.data +
                                   platform.permanentMemory.size);

  // Api bindings
  platform.api.initDisplay = initDisplay;
  platform.api.renderBegin = rendererBegin;
  platform.api.renderEnd = rendererFlip;
  platform.api.renderMesh = renderMesh;
  platform.api.renderMeshInstanced = renderMeshInstances;
  platform.api.log = SDL_LOG;
  platform.api.assert = SDL_ASSERT_;
  platform.api.diskIOReadFile = diskIOReadFile;
  platform.api.diskIOReadModTime = fileReadModTime;
  platform.api.initMeshBuffers = initMeshBuffers;
  platform.api.initMaterialBuffers = initMaterialBuffers;
  platform.api.initShaderProgram = initShaderProgram;
  platform.api.updateMaterialBuffers = updateMaterialBuffers;
  platform.api.freeMeshBuffers = freeMeshBuffers;
  platform.api.freeMaterialBuffers = freeMaterialBuffers;
  platform.api.freeShaderProgram = freeShaderProgram;

  input_state input;
  inputInit(&input);

  game_code gameCode;
  gameCode.libGameCode = NULL;
  const char *gameCodeLib = "./librottengame.so";
  SDLLoadGameCode(gameCodeLib, &gameCode, fileReadModTime(gameCodeLib));

  gameCode.onStartFunc(&platform);

  bool quit = false;

  uint32_t t = 0.0;
  uint32_t targetTimeMSec = 1000 / 60;

  uint32_t currentTime = SDL_GetTicks();
  uint32_t elapsedTimeMSec = 0;

  while (!quit) {
    time_t wt = fileReadModTime("./librottengame.so");
    if (wt != 0 && wt > gameCode.libLastWriteTime) {
      // TODO: Haven't find (yet) a reliable way to determine if file is still
      // being modified.
      // Currently bypassing this by compiling to temp.so file, then post
      // build copy the so file to its proper place.
      if (SDLIsFileReady("./librottengame.so")) {
        SDLUnloadObject(&gameCode);
        SDLLoadGameCode("./librottengame.so", &gameCode, wt);
      } else {
        sleep(1);
      }
    }

    uint32_t newTime = SDL_GetTicks();
    uint32_t frameTimeMS = newTime - currentTime;
    currentTime = newTime;

    t += frameTimeMS;

    if (frameTimeMS > targetTimeMSec) {
      frameTimeMS = targetTimeMSec;
    }

    elapsedTimeMSec += frameTimeMS;

    while (elapsedTimeMSec >= targetTimeMSec) {
      elapsedTimeMSec -= targetTimeMSec;
      float dt = (float)targetTimeMSec / 1000.f;
      inputParseEvents(&input);
      // quit = gameCode.gameLoopFunc(dt, gameState);
      quit = gameCode.gameLoopFunc(dt, t, &platform, &input);

      imGuiUpdate();
      imGuiRender();
    }
  }
  imGuiShutdown();

  rendererClose();

  free(permanentMem);

  SDL_Quit();

  return 0;
}
