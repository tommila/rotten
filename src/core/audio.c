#define STB_VORBIS_NO_STDIO

#include "../ext/stb_vorbis.c"

rt_audio_data loadOGG(const char* path) {
  rt_audio_data audio;

  usize audioFileSize = SDLReadFileSizeB(path);
  if (audioFileSize > 0) {
    void* fileBuffer = (void*)pushSize(&platformMemArena, audioFileSize);
    SDLReadFileBTo(path, audioFileSize, fileBuffer);
    i32 sampleNum = stb_vorbis_decode_memory(
      (u8*)fileBuffer, audioFileSize,
      &audio.channels, &audio.sampleRate,
      &audio.buffer);
    audio.sampleNum = sampleNum;
  }
  return audio;
}
