#pragma once

#include <Windows.h>

struct Module {
  union {
    HMODULE hModule;
    DWORD dwBaseAddress;
  };
  DWORD _1;
  wchar_t szPath[MAX_PATH];
};
