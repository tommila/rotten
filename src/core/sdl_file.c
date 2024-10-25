static int64_t SDLFileSize(SDL_RWops* f) {
  SDL_RWseek(f, 0l, RW_SEEK_END);
  int64_t size = SDL_RWtell(f);
  SDL_RWseek(f, 0l, RW_SEEK_SET);
  return size;
}

b32 SDLFileExists(const char* filePath) {
  b32 result = false;
  SDL_RWops* f = SDL_RWFromFile(filePath, "r");
  result = f != 0;
  if (result) {
    SDL_RWclose(f);
  }
  return result;
}

static void* _SDLReadFile(const char* filePath, u32* size,
                          const char* mode) {
  LOG(LOG_LEVEL_DEBUG, "Read file %s", filePath);
  SDL_RWops* f = SDL_RWFromFile(filePath, mode);
  void* data = NULL;
  if (f != NULL) {
    *size = SDLFileSize(f) + 1;
    data = malloc(*size);
    if (SDL_RWread(f, data, *size - 1, 1) == 0) {
      LOG(LOG_LEVEL_ERROR, "File load failed: %s", SDL_GetError());
      free(data);
      data = NULL;
    }
    SDL_RWclose(f);
    ((char*)data)[*size - 1] = '\0';
  } else {
    LOG(LOG_LEVEL_ERROR, "File load failed %s", SDL_GetError());
  }

  return data;
}

#define SDLReadFile(filePath, size) (_SDLReadFile(filePath, size, "r"))
#define SDLReadFileB(filePath, size) (_SDLReadFile(filePath, size, "rb"))

static void _SDLReadFileTo(const char* filePath, u32 dataSize, void* dataOut,
                           const char* mode) {
  LOG(LOG_LEVEL_DEBUG, "Read file %s", filePath);
  SDL_RWops* f = SDL_RWFromFile(filePath, mode);
  if (f != NULL) {
    if (SDL_RWread(f, dataOut, dataSize, 1) == 0) {
      LOG(LOG_LEVEL_ERROR, "File load failed: %s", SDL_GetError());
    }
    SDL_RWclose(f);
    ((char*)dataOut)[dataSize] = '\0';
  } else {
    LOG(LOG_LEVEL_ERROR, "File %s load failed %s", filePath, SDL_GetError());
  }
}

#define SDLReadFileTo(filePath, dataSize, dataOut) \
  (_SDLReadFileTo(filePath, dataSize, dataOut, "r"))
#define SDLReadFileBTo(filePath, dataSize, dataOut) \
  (_SDLReadFileTo(filePath, dataSize, dataOut, "rb"))

static u32 _SDLReadFileSize(const char* filePath, const char* mode) {
  LOG(LOG_LEVEL_DEBUG, "Read file size %s", filePath);
  SDL_RWops* f = SDL_RWFromFile(filePath, mode);
  u32 size = 0;
  if (f != NULL) {
    size = SDLFileSize(f);
    SDL_RWclose(f);

  } else {
    LOG(LOG_LEVEL_ERROR, "File %s load failed %s", filePath, SDL_GetError());
  }

  return size;
}

#define SDLReadFileSize(filePath) (_SDLReadFileSize(filePath, "r"))
#define SDLReadFileSizeB(filePath) (_SDLReadFileSize(filePath, "rb"))

b32 SDLWriteFile(const char* filePath, void* data, u32 size) {
  LOG(LOG_LEVEL_DEBUG, "Write file %s", filePath);
  b32 success = false;
  SDL_RWops* f = SDL_RWFromFile(filePath, "wb");
  if (f) {
    u32 ret = SDL_RWwrite(f, data, size, 1);
    if (ret < 1) {
      LOG(LOG_LEVEL_ERROR, "File write error: %s", SDL_GetError());
    }
    success = true;
    SDL_RWclose(f);
  } else {
    LOG(LOG_LEVEL_ERROR, "File write failed: %s", SDL_GetError());
  }
  return success;
}

b32 SDLCopyFile(const char* fromPath, const char* toPath) {
  LOG(LOG_LEVEL_DEBUG, "Copy file %s to %s", fromPath, toPath);
  b32 ret = false;
  u32 size;
  void* data = SDLReadFile(fromPath, &size);
  if (data) {
    ret = SDLWriteFile(toPath, data, size);
    free(data);
  }

  if (!ret) {
    LOG(LOG_LEVEL_ERROR, "File copy failed %s", SDL_GetError());
  }

  return ret;
}
