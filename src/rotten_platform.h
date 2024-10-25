typedef enum rt_key_mod {
  rt_key_mod_left_alt = 1 << 0,
  rt_key_mod_left_shift = 1 << 1
  
} rt_key_mod;

typedef struct rt_key_event {
  b32 pressed;
  char keycode[16];
  rt_key_mod mod;
} rt_key_event;

typedef struct rt_mouse_event {
  i32 position[2];
  i32 delta[2];
  i32 wheel;
} rt_mouse_event;

typedef struct rt_mouse_button_event {
  i32 buttonIdx;
  b32 pressed;
} rt_mouse_button_event;

typedef enum rt_input_type {
  rt_input_type_none = 0,
  rt_input_type_key,
  rt_input_type_mouse,
  rt_input_type_mouse_button
} rt_input_type;

typedef struct rt_input_event {
  rt_input_type type;
  union {
    rt_key_event key;
    rt_mouse_event mouse;
    rt_mouse_button_event mouseButton;
  };
} rt_input_event;

typedef struct rt_input {
  rt_input_event* events;
  u32 eventNum;
} rt_input;

typedef struct rt_audio_data {
  i16* buffer;
  i32 sampleNum;
  i32 sampleRate;
  i32 channels;
} rt_audio_data;

#define LOGGER_PTR(name) void (*name)(LogLevel logLevel, const char* txt, ...)
typedef LOGGER_PTR(loggerPtr);
#define ASSERT_PTR(name)						       \
  void (*name)(b32 cond, const char* condText, const char* function, \
                 int linenum, const char* filename)
typedef ASSERT_PTR(assertPtr);

typedef struct platform_api {
  assertPtr assert;
  loggerPtr logger;

  void (*toggleFullscreen)();

  void (*flushCommandBuffer)(rt_command_buffer* buffer);

  u64 (*getPerformanceCounter)();
  u64 (*getPerformanceFrequency)();

  time_t (*readFileModTime)(const char* path);
  void* (*loadBinaryFile)(const char* path, usize* fileSizeOut);
  char* (*loadTextFile)(const char* path, usize* fileSizeOut);
  rt_audio_data (*loadOGG)(const char* path);
  rt_image_data (*loadImage)(const char* path, u8 channels);
} platform_api;

typedef struct platform_state {
  void* permanentMemBuffer;
  u32 permanentMemSize;
  void* temporaryMemBuffer;
  u32 temporaryMemSize;
  platform_api api;
} platform_state;

typedef struct display_state {
  v2i size; 
} display_state;

typedef struct frame_time {
  f32 delta;
  f32 duration;
} frame_time;

typedef struct audio_out {
  u8* buffer;
  i32 size;
  i32 samplesPerSecond;
  i32 sampleCount;
  i32 bytesPerSample;
} audio_out;

typedef struct audio_buffer {
  u8* buffer;
  i32 size;
  i32 readCursor;
  i32 writeCursor;
  i32 bytesPerSample;
  i32 bytesToBeWritten;
  i32 sampleIndex;
} audio_buffer;

// Game code //
#ifndef __cplusplus
b32 gameUpdate(frame_time time, display_state display, platform_state platform,
               rt_input input,
               b32 reloaded, utime assetModTime);
void audioCallback(audio_out audioOut, platform_state platform);

#endif
#define RT_GAME_UPDATE_AND_RENDER(name)                       \
  b32 name(frame_time time, display_state display, platform_state platform, \
            rt_input input,\
            b32 reloaded, utime assetModTime)
#define RT_GAME_AUDIO_CALLBACK(name)                       \
  void name(audio_out audioOut, platform_state platform)

typedef RT_GAME_UPDATE_AND_RENDER(rt_gameUpdate);
typedef RT_GAME_AUDIO_CALLBACK(rt_gameAudioUpdate);
