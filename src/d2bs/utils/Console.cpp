#include "d2bs/utils/Console.h"

#include "d2bs/core/Core.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/utils/Helpers.h"

#include <sstream>
#include <string>

void Console::Toggle(void) {
  ToggleBuffer();
  TogglePrompt();
}

void Console::TogglePrompt(void) {
  if (!IsEnabled())
    ShowPrompt();
  else
    HidePrompt();
}

void Console::ToggleBuffer(void) {
  if (!IsVisible())
    ShowBuffer();
  else
    HideBuffer();
}

void Console::Hide(void) {
  HidePrompt();
  HideBuffer();
}

void Console::HidePrompt(void) {
  std::lock_guard<std::mutex> lock(mutex_);
  enabled = false;
}

void Console::HideBuffer(void) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    visible = false;
  }

  if (IsEnabled()) {
    HidePrompt();
  }
}

void Console::Show(void) {
  ShowBuffer();
  ShowPrompt();
}

void Console::ShowPrompt(void) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled = true;
  }

  if (!IsVisible()) {
    ShowBuffer();
  }
}

void Console::ShowBuffer(void) {
  std::lock_guard<std::mutex> lock(mutex_);
  visible = true;
}

void Console::AddKey(unsigned int key) {
  std::lock_guard<std::mutex> lock(mutex_);
  cmd << (char)key;
}

void Console::ExecuteCommand(void) {
  if (cmd.str().length() < 1) {
    return;
  }

  commands.push_back(cmd.str());
  commandPos = commands.size();
  ProcessCommand(cmd.str().c_str(), true);
  cmd.str(L"");
}

void Console::RemoveLastKey(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  int len = cmd.str().length() - 1;
  if (len >= 0) {
    cmd.str(cmd.str().substr(0, len));
    if (len > 0) {
      cmd.seekg(len);
      cmd.seekp(len);
    }
  }
}

void Console::PrevCommand(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (commandPos < 1 || commandPos > commands.size()) {
    cmd.str(L"");
  } else {
    if (commandPos >= 1) commandPos--;
    cmd.str(L"");
    cmd << commands[commandPos];
  }
}

void Console::NextCommand(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (commandPos >= commands.size()) {
    return;
  }

  cmd.str(L"");
  cmd << commands[commandPos];

  if (commandPos < commands.size() - 1) {
    commandPos++;
  }
}

void Console::ScrollUp(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (scrollIndex == 0 || history.size() - scrollIndex == 0) {
    return;
  }

  scrollIndex--;
  Console::UpdateLines();
}

void Console::ScrollDown(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (history.size() < lineCount || (history.size() - lineCount == scrollIndex)) {
    return;
  }

  scrollIndex++;
  Console::UpdateLines();
}

void Console::AddLine(std::wstring line) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::list<std::wstring> buf;
  SplitLines(line, Console::MaxWidth(), L' ', buf);
  for (std::list<std::wstring>::iterator it2 = buf.begin(); it2 != buf.end(); it2++) {
    history.push_back(*it2);
  }

  while (history.size() > 300)  // set history cap at 300
    history.pop_front();

  if (Vars.bLogConsole) {
    LogNoFormat(line.c_str());
  }

  scrollIndex = history.size() < lineCount ? 0 : history.size() - lineCount;
  Console::UpdateLines();
}

void Console::UpdateLines(void) {
  lines.clear();
  for (int j = history.size() - scrollIndex; j > 0 && lines.size() < lineCount; j--)
    lines.push_back(history.at(history.size() - j));
}

void Console::Clear(void) {
  std::lock_guard<std::mutex> lock(mutex_);
  lines.clear();
}

void Console::Draw(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  static DWORD count = GetTickCount();
  if (IsVisible()) {
    POINT size = GetScreenSize();
    int xsize = size.x;
    int ysize = size.y;
    size = CalculateTextLen("@", Vars.dwConsoleFont);
    int charwidth = size.x;
    int charheight = max(12, size.y / 2 + 2);
    // the default console height is 30% of the screen size
    int _height = ((int)(((double)ysize) * .3) / charheight) * charheight + charheight;
    lineWidth = xsize - (2 * charwidth);
    lineCount = _height / charheight;

    int cmdsize = 0;
    int cmdlines = 0;
    std::list<std::wstring> cmdsplit;
    if (IsEnabled()) {
      std::wstring cmdstr = cmd.str();
      if (cmdstr.length() > 0) {
        SplitLines(cmdstr, Console::MaxWidth(), L' ', cmdsplit);
        cmdsize = CalculateTextLen(cmdsplit.back().c_str(), Vars.dwConsoleFont).x;
        cmdlines += cmdsplit.size() - 1;
      }
    }

    Console::height = _height + (cmdlines * charheight) + 6;
    // draw the box large enough to hold the whole thing
    D2GFX_DrawRectangle(0, 0, xsize, Console::height, 0xdf, 0);

    std::deque<std::wstring>::reverse_iterator it = lines.rbegin();
    if (scrollIndex == 0 && lines.size() == lineCount && IsEnabled())  // handle index 0, top of console
      it++;

    for (int i = lineCount - (int)IsEnabled(); i > 0 && it != lines.rend(); i--, it++)
      myDrawText(it->c_str(), charwidth, 4 + (i * charheight), 0, Vars.dwConsoleFont);

    if (IsEnabled()) {
      if (cmdsplit.size() > 0) {
        int dy = _height + 3;
        for (std::list<std::wstring>::iterator it2 = cmdsplit.begin(); it2 != cmdsplit.end(); it2++, dy += charheight) {
          myDrawText(it2->c_str(), charwidth, dy, 0, Vars.dwConsoleFont);
        }
      }

      myDrawText(L">", 1, Console::height - 3, 0, Vars.dwConsoleFont);
      DWORD tick = GetTickCount();
      if ((tick - count) < 600) {
        int lx = cmdsize + charwidth, ly = Console::height - (charheight / 3);
        D2GFX_DrawRectangle(lx, ly, lx + ((charwidth * 2) / 3), ly + 2, 0xFF, 0x07);
      } else if ((tick - count) > 1100) {
        count = tick;
      }
    }
  }
}
