#include "../platform.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "imgui-impl.cpp"

void inputInit(input_state* out) {
  out->downButton.sym = SDLK_s;
  out->upButton.sym = SDLK_w;
  out->leftButton.sym = SDLK_a;
  out->rightButton.sym = SDLK_d;

  out->pauseButton.sym = SDLK_p;
  out->gridButton.sym = SDLK_g;
  out->quitButton.sym = SDLK_ESCAPE;
  out->timeStepButton.sym = SDLK_SPACE;

  for (size_t it = 0; it < 8; it++) {
    out->allButtons[it].state = Up;
  }
}

void inputParseEvents(input_state* input) {
  input->wheel = 0.0;

  for (size_t it = 0; it < 8; it++) {
    if (input->allButtons[it].state == Pressed) {
      input->allButtons[it].state = Held;
    } else if (input->allButtons[it].state == Released) {
      input->allButtons[it].state = Up;
    }
  }

  SDL_StartTextInput();
  for (SDL_Event event; SDL_PollEvent(&event) != 0;) {
    if (imGuiProcessEvent(&event)) {
      continue;
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      for (size_t it = 0; it < 8; it++) {
        if (input->allButtons[it].sym == event.key.keysym.sym) {
          input->allButtons[it].state =
              event.type == SDL_KEYDOWN ? Pressed : Released;
        }
      }
    }

    if (event.type == SDL_MOUSEWHEEL) {
      input->wheel = event.wheel.y;
    }
    if (event.type == SDL_QUIT) {
      input->quitButton.state = Pressed;
    }

    input->axis_state[0] = float(input->leftButton.state >= Pressed) * -1 +
                           float(input->rightButton.state >= Pressed) * 1;
    input->axis_state[1] = float(input->upButton.state >= Pressed) * 1 +
                           float(input->downButton.state >= Pressed) * -1;
  }
  SDL_StopTextInput();
}
