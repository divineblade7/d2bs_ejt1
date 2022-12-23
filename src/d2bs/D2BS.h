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

 private:
  friend DWORD WINAPI thread_entry(void* param);
  void parse_commandline_args();

  void init_paths(HMODULE mod);
  void init_settings();

 private:
  HANDLE thread_handle_ = INVALID_HANDLE_VALUE;
  bool initialized_ = false;

  fs::path root_dir_;
  fs::path logs_dir_;
  fs::path settings_file_;
};

#define sEngine D2BS::instance()
