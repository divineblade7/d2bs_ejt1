#pragma once

#include "d2bs/D2BS.h"

#include <deque>
#include <mutex>
#include <sstream>
#include <string>
#include <windows.h>

class Console {
  Console() noexcept = default;
  ~Console() noexcept = default;

 public:
  static Console* instance() {
    static Console _instance;
    return &_instance;
  }

  Console(const Console&) = delete;
  Console& operator=(const Console&) = delete;

  void Toggle(void);
  void TogglePrompt(void);
  void ToggleBuffer(void);
  void Hide(void);
  void HidePrompt(void);
  void HideBuffer(void);
  void Show(void);
  void ShowPrompt(void);
  void ShowBuffer(void);

  void AddKey(unsigned int key);
  void ExecuteCommand(void);
  void RemoveLastKey(void);
  void PrevCommand(void);
  void NextCommand(void);
  void ScrollUp(void);
  void ScrollDown(void);
  void AddLine(std::wstring line);
  void UpdateLines(void);
  void Clear(void);
  void Draw(void);

  bool IsVisible(void) {
    return visible;
  }

  bool IsEnabled(void) {
    return enabled;
  }

  unsigned int MaxWidth(void) {
    return lineWidth;
  }

  unsigned int GetHeight(void) {
    return height;
  }

 private:
  bool visible = false;
  bool enabled = false;
  std::deque<std::wstring> lines, commands, history;
  unsigned int lineCount = 14;
  unsigned int lineWidth = 625;
  unsigned int commandPos = 0;
  unsigned int height = 0;
  unsigned int scrollIndex = 0;
  std::wstringstream cmd;

  std::mutex mutex_;
};

#define sConsole Console::instance()
