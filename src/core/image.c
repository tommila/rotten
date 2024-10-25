#define STBI_ASSERT ASSERT

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(size) memArena_alloc(&platformMemArena, size)
#define STBI_REALLOC(p, newSize) memArena_realloc(&platformMemArena, p, newSize)
#define STBI_FREE(p) {}

#include "../ext/stb_image.h"

rt_image_data loadImage(const char* path, u8 channelNum) {   
  rt_image_data imgData = {};
  usize pngSize = SDLReadFileSizeB(path);
  if (pngSize > 0) {
    void* pngData = (void*)pushSize(&platformMemArena, pngSize);
    SDLReadFileBTo(path, pngSize, pngData);
    int x, y, n;
    stbi_uc *pixels = stbi_load_from_memory(
      (stbi_uc *)pngData, (i32)pngSize, &x, &y, &n, channelNum);
    if (pixels == NULL) {
      LOG(LOG_LEVEL_ERROR, "File load error %s", stbi_failure_reason());
      ASSERT(0);
    }

    imgData.pixels = pixels;
    imgData.dataSize = x * y * 4;
    imgData.width = x;
    imgData.height = y;
    imgData.depth = 3;
    imgData.components = channelNum;
    
  }

  return imgData;
}
