#pragma once

#ifndef XP_WIN
#define XP_WIN
#endif

// ScreenHook.h does a lot of implicity inclusions which causes compilation errors if moved into D2BS.cpp
// fix this...
#include "d2bs/core/ScreenHook.h"
#include "d2bs/variables.h"
#include "d2bs/version.h"

#include <Windows.h>
#include <filesystem>
#include <queue>
#include <vector>

namespace fs = std::filesystem;

namespace d2bs {

class Engine {
  Engine() noexcept = default;
  ~Engine() noexcept = default;

 public:
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  static Engine* instance() {
    static Engine _instance;
    return &_instance;
  }

  bool startup(HMODULE mod);
  void shutdown(bool await_thread = false);

  void update();

  void on_game_enter();

  void on_game_draw();
  void on_menu_draw();

  ScriptEngine* script_engine() {
    return &script_engine_;
  }

 private:
  void run_chicken();

  friend DWORD WINAPI thread_entry(void* param);
  friend LONG WINAPI wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  void parse_commandline_args();

  void init_paths(HMODULE mod);
  void init_settings();
  bool init_hooks();

 private:
  ScriptEngine script_engine_;

  HANDLE thread_handle_ = INVALID_HANDLE_VALUE;
  WNDPROC orig_wndproc_ = nullptr;
  bool initialized_ = false;
  bool first_menu_call_ = true;

  fs::path root_dir_;
  fs::path logs_dir_;
  fs::path settings_file_;
};

}  // namespace d2bs

#define sEngine d2bs::Engine::instance()
