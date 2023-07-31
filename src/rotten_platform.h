
typedef struct rt_key_event {
  b32 pressed;
  char keycode[16];
} rt_key_event;

typedef struct rt_mouse_event {
  i32 buttonIdx;
  b32 pressed;
  i32 position[2];
  i32 wheel;
} rt_mouse_event;

typedef enum rt_input_type {
  RT_INPUT_TYPE_NONE = 0,
  RT_INPUT_TYPE_KEY,
  RT_INPUT_TYPE_MOUSE
} rt_input_type;

typedef struct rt_input_event {
  rt_input_type type;
  union {
    rt_key_event key;
    rt_mouse_event mouse;
  };
} rt_input_event;

#define LOGGER_PTR(name) void (*name)(LogLevel logLevel, const char* txt, ...)
typedef LOGGER_PTR(loggerPtr);
#define ASSERT_PTR(name)						       \
  void (*name)(b32 cond, const char* condText, const char* function, \
                 int linenum, const char* filename)
typedef ASSERT_PTR(assertPtr);

typedef struct platform_state platform_state;
typedef struct platform_api {
  assertPtr assert;
  loggerPtr logger;
  void (*renderer_flushCommandBuffer)(rt_render_entry_buffer* buffer);

  u64 (*getPerformanceCounter)();
  u64 (*getPerformanceFrequency)();

  void* (*diskIOReadFile)(const char* path, u32* size, b32 isBinary);
  void (*diskIOReadFileTo)(const char* path, u32 dataSize, b32 isBinary,
                           void* dataOut);
  u32 (*diskIOReadFileSize)(const char* path, b32 isBinary);
  time_t (*diskIOReadModTime)(const char* path);
} platform_api;

struct platform_state {
  void* permanentMemBuffer;
  u32 permanentMemSize;
  void* transientMemBuffer;
  u32 transientMemSize;
  platform_api api;
};

// Game code //
#ifndef __cplusplus
b32 gameUpdate(f32 delta, f32 duration, platform_state* platform,
                          rt_input_event* inputEvents, b32 reloaded, utime assetModTime);
#endif
#define RT_GAME_UPDATE_AND_RENDER(name)                       \
  b32 name(f32 delta, f32 duration, platform_state* platform, \
           rt_input_event* inputEvents, b32 reloaded, utime assetModTime)
typedef RT_GAME_UPDATE_AND_RENDER(rt_gameUpdate);
