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
