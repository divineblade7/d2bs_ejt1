#pragma once

#ifndef XP_WIN
#define XP_WIN
#endif

// ScreenHook.h does a lot of implicity inclusions which causes compilation errors if moved into D2BS.cpp
// fix this...
#include "d2bs/core/ScreenHook.h"
#include "d2bs/utils/dde.h"
#include "d2bs/variables.h"
#include "d2bs/version.h"

#include <Windows.h>
#include <filesystem>
#include <queue>
#include <vector>

namespace fs = std::filesystem;

class D2BS {
  D2BS() noexcept = default;
  ~D2BS() noexcept = default;

 public:
  D2BS(const D2BS&) = delete;
  D2BS& operator=(const D2BS&) = delete;

  static D2BS* instance() {
    static D2BS _instance;
    return &_instance;
  }

  bool startup(HMODULE mod);
  void shutdown(bool await_thread = false);

  void update();

  void on_game_enter();
  void on_menu_enter();

  ScriptEngine* script_engine() {
    return &script_engine_;
  }

  DdeServer* dde() {
    return &dde_;
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
  DdeServer dde_;

  HANDLE thread_handle_ = INVALID_HANDLE_VALUE;
  WNDPROC orig_wndproc_ = nullptr;
  bool initialized_ = false;
  bool first_menu_call_ = true;

  fs::path root_dir_;
  fs::path logs_dir_;
  fs::path settings_file_;
};

#define sEngine D2BS::instance()
