#include <SDL.h>

#include <cstdint>

#include "../platform.h"
#include "SDL_surface.h"

static int64_t SDLFileSize(SDL_RWops* f) {
  int64_t size = SDL_RWseek(f, 0l, RW_SEEK_END);
  SDL_RWseek(f, 0l, RW_SEEK_SET);
  return size;
}

bool SDLIsFileReady(const char* filePath) {
  bool result = false;
  SDL_RWops* f = SDL_RWFromFile(filePath, "r+");
  if (f) {
    int64_t size = SDLFileSize(f);
    result = size > 0l;
    SDL_RWclose(f);
  }
  return result;
}

void* SDLTryReadFile(int tryNum, uint32_t sleep, const char* filePath,
                     size_t* size) {
  SDL_LOG(LOG_LEVEL_VERBOSE, "Try reading file '%s'-\n", filePath);
  int i = tryNum;
  void* data = NULL;
  while (i > 0) {
    SDL_RWops* f = SDL_RWFromFile(filePath, "rb");
    if (f != NULL) {
      *size = SDLFileSize(f);
      data = malloc(*size);
      if (SDL_RWread(f, data, *size, 1) == 0) {
        SDL_LOG(LOG_LEVEL_WARN, "File load failed: '%s'\n", SDL_GetError());
        free(data);
        data = NULL;
      }
      SDL_RWclose(f);
      if (data) {
        break;
      }
    }
    SDL_Delay(sleep);
    i;
  }
  return data;
}

static void* _SDLReadFile(const char* filePath, size_t* size,
                          const char* mode) {
  SDL_LOG(LOG_LEVEL_INFO, "Read file %s\n", filePath);
  SDL_RWops* f = SDL_RWFromFile(filePath, mode);
  void* data = NULL;
  if (f != NULL) {
    *size = SDLFileSize(f);
    data = malloc(*size);
    if (SDL_RWread(f, data, *size, 1) == 0) {
      SDL_LOG(LOG_LEVEL_ERROR, "File load failed: %s\n", SDL_GetError());
      free(data);
      data = NULL;
    }
    SDL_RWclose(f);
  } else {
    SDL_LOG(LOG_LEVEL_ERROR, "File load failed %s\n", SDL_GetError());
  }

  return data;
}

#define SDLReadFile(filePath, size) (_SDLReadFile(filePath, size, "r"))
#define SDLReadFileB(filePath, size) (_SDLReadFile(filePath, size, "rb"))

bool SDLWriteFile(const char* filePath, void* data, size_t size) {
  SDL_LOG(LOG_LEVEL_INFO, "Write file %s\n", filePath);
  bool success = false;
  SDL_RWops* f = SDL_RWFromFile(filePath, "wb");
  if (f) {
    size_t ret = SDL_RWwrite(f, data, size, 1);
    if (ret < 1) {
      SDL_LOG(LOG_LEVEL_ERROR, "File write error: %s\n", SDL_GetError());
    }
    success = true;
    SDL_RWclose(f);
  } else {
    SDL_LOG(LOG_LEVEL_ERROR, "File write failed: %s\n", SDL_GetError());
  }
  return success;
}

bool SDLCopyFile(const char* fromPath, const char* toPath) {
  SDL_LOG(LOG_LEVEL_INFO, "Copy file %s to %s\n", fromPath, toPath);
  bool ret = false;
  size_t size;
  void* data = SDLReadFile(fromPath, &size);
  if (data) {
    ret = SDLWriteFile(toPath, data, size);
    free(data);
  }

  if (!ret) {
    SDL_LOG(LOG_LEVEL_ERROR, "File copy failed %s\n", SDL_GetError());
  }

  return ret;
}
