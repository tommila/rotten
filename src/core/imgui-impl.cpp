#include <SDL.h>
#include <bgfx/bgfx.h>
#include <imgui.h>

#include <cstdio>

#include "../ext/bgfx-imgui/imgui_impl_bgfx.cpp"
#include "../ext/sdl-imgui/imgui_impl_sdl2.cpp"

bool imGuiImplInitSDL(SDL_Window *window) {
  ImGui::CreateContext();

  ImGui_Implbgfx_Init(255);
#if __WINDOWS__
  return ImGui_ImplSDL2_InitForD3D(window);
#elif __MACOSX__
  return ImGui_ImplSDL2_InitForMetal(window);
#elif __LINUX__ || __NETBSD__ || __OPENBSD__
  return ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif  // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
        // BX_PLATFORM_EMSCRIPTEN
  return false;
}

void imGuiImplInitBgfx() { ImGui_Implbgfx_Init(255); }

bool p_open = true;

bool imGuiImplProcessEvent(SDL_Event *event) {
  ImGui_ImplSDL2_ProcessEvent(event);
  return ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse;
}

void imGuiImplUpdate() {
  ImGui_Implbgfx_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(
      ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y),
      ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(256, 480), ImGuiCond_FirstUseEver);
  p_open = ImGui::Begin("Stats", &p_open);

  if (!p_open) {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }
}

void imGuiImplRender() {
  if (p_open) {
    ImGui::End();
  }
  ImGui::Render();
  ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
}

void imGuiImplShutdown() {
  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
}

bool imGuiImplIsWindowOpen() { return p_open; }
