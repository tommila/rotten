#include <unistd.h>

#include "../platform.h"
#include "SDL.h"

void SDLUnloadObject(game_code *code) { SDL_UnloadObject(code->libGameCode); }

bool SDLLoadGameCode(const char *libName, game_code *code, time_t wt) {
  SDL_LOG(LOG_LEVEL_INFO, "Load Game code\n");
  code->libLastWriteTime = wt;

  code->isValid = false;
  code->libGameCode = SDL_LoadObject(libName);
  if (code->libGameCode) {
    code->onStartFunc =
        (game_on_start *)SDL_LoadFunction(code->libGameCode, "gameOnStart");
    code->gameLoopFunc = (game_update_and_render *)SDL_LoadFunction(
        code->libGameCode, "gameUpdateAndRender");

    code->isValid = code->onStartFunc && code->gameLoopFunc;
  }
  if (!code->isValid) {
    SDL_LOG(LOG_LEVEL_ERROR, "Could not load game code: %s\n", SDL_GetError());
    assert(false);
  }

  return code;
}
