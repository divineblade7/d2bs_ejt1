#pragma once

#include "d2bs/diablo/D2Structs.h"  // CellFile
#include "d2bs/new_core/settings.h" // Settings

#include <Windows.h>   // CRITICAL_SECTION, DWORD, HANDLE, HHOOK, HMODULE, POINT, UINT_PTR, WNDPROC
#include <filesystem>  // std::filesystem::path
#include <map>         // std::map
#include <queue>       // std::queue
#include <string>      // std::wstring
#include <utility>     // std::pair
#include <vector>      // std::vector

// Temporarily adding these here until I can untangle the great web och inclusions.
// ArraySize, PRIVATE_ITEM, PRIVATE_UNIT and Private should be moved. ~ ejt
#define ArraySize(x) (sizeof((x)) / sizeof((x)[0]))

#define PRIVATE_UNIT 1
#define PRIVATE_ITEM 3

struct Private {
  DWORD dwPrivateType;
};

struct Variables {
  // Temporarily defining settings here to make it globally available
  d2bs::Settings settings;

  int nChickenHP;
  int nChickenMP;
  BOOL bDontCatchNextMsg;
  BOOL bClickAction;
  BOOL bChangedAct;
  BOOL bGameLoopEntered;
  DWORD dwGameThreadId;
  DWORD dwLocale;

  BOOL bTakeScreenshot;
  BOOL bActive;
  BOOL bBlockKeys;
  BOOL bBlockMouse;
  BOOL bUseRawCDKey = FALSE;
  BOOL bQuitting;
  BOOL bCacheFix;
  BOOL bMulti;
  BOOL bSleepy;
  BOOL bReduceFTJ;
  HMODULE hModule;
  HWND hHandle;

  std::filesystem::path working_dir;
  std::filesystem::path log_dir;
  char szClassic[30];
  char szLod[30];
  wchar_t szTitle[256];

  HHOOK hMouseHook;
  HHOOK hKeybHook;

  UINT_PTR uTimer;
  long SectionCount;

  std::queue<std::wstring> qPrintBuffer;
  std::map<unsigned __int32, CellFile*> mCachedCellFiles;
  // std::list<Event*> EventList;
  CRITICAL_SECTION cEventSection;
  CRITICAL_SECTION cPrintSection;
  CRITICAL_SECTION cTextHookSection;
  CRITICAL_SECTION cImageHookSection;
  CRITICAL_SECTION cBoxHookSection;
  CRITICAL_SECTION cFrameHookSection;
  CRITICAL_SECTION cLineHookSection;
  CRITICAL_SECTION cFlushCacheSection;
  CRITICAL_SECTION cGameLoopSection;
  CRITICAL_SECTION cFileSection;

  DWORD dwSelectedUnitId;
  DWORD dwSelectedUnitType;
  POINT pMouseCoords;
};

extern Variables Vars;
