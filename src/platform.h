#pragma once

#define BX_CONFIG_DEBUG 1
#include <stdarg.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>

#include "SDL2/SDL_assert.h"
#include "SDL2/SDL_log.h"
#include "cglm/types.h"
#include "rotten_platform.h"
#define CONCAT2(dest, str1, str2) \
  strcpy(dest, str1);             \
  strcat(dest, str2);             \
  dest[strlen(str1) + strlen(str2)] = '\0'
#define CONCAT3(dest, str1, str2, str3) \
  strcpy(dest, str1);                   \
  strcat(strcat(dest, str2), str3);     \
  dest[strlen(str1) + strlen(str2) + strlen(str3)] = '\0'
#define CONCAT4(dest, str1, str2, str3, str4)     \
  strcpy(dest, str1);                             \
  strcat(strcat(strcat(dest, str2), str3), str4); \
  dest[strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4)] = '\0'
#define CONCAT5(dest, str1, str2, str3, str4, str5)                \
  strcpy(dest, str1);                                              \
  strcat(strcat(strcat(strcat(dest, str2), str3), str4), str5);    \
  dest[strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + \
       strlen(str5)] = '\0'

static inline void SDL_LOG(LogLevel logLevel, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, (SDL_LogPriority)logLevel,
                  format, ap);
  va_end(ap);
}

#define SDL_ASSERT(cond) \
  SDL_ASSERT_(cond, #cond, SDL_FUNCTION, SDL_LINE, SDL_FILE)
// Stupid wrapper
inline void SDL_ASSERT_(bool cond, const char* condText, const char* function,
                        int linenum, const char* filename) {
  do {
    while (!(cond)) {
      static struct SDL_AssertData sdl_assert_data = {0, 0, condText, 0,
                                                      0, 0, 0};
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

#ifndef IMGUI_DISABLED
#define imGuiInitSDL(window) imGuiImplInitSDL(window)
#define imGuiInitBgfx imGuiImplInitBgfx
#define imGuiProcessEvent(event) imGuiImplProcessEvent(event)
#define imGuiUpdate imGuiImplUpdate
#define imGuiRender imGuiImplRender
#define imGuiShutdown imGuiImplShutdown
#define imGuiIsWindowOpen imGuiImplIsWindowOpen
#else
#define imGuiInitSDL(window) (true)
#define imGuiInitBgfx() (true)
#define imGuiProcessEvent(event) (false)
#define imGuiUpdate()
#define imGuiRender()
#define imGuiShutdown()
#define imGuiIsWindowOpen()
#endif
