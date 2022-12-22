#pragma once

#include "d2bs/cguard.h"            // Module
#include "d2bs/diablo/D2Structs.h"  // CellFile

#include <Windows.h>  // CRITICAL_SECTION, DWORD, HANDLE, HHOOK, HMODULE, POINT, UINT_PTR, WNDPROC
#include <map>        // std::map
#include <queue>      // std::queue
#include <string>     // std::wstring
#include <filesystem> // std::filesystem::path
#include <vector>     // std::vector
#include <utility>    // std::pair

// Temporarily adding these here until I can untangle the great web och inclusions.
// ArraySize, PRIVATE_ITEM, PRIVATE_UNIT and Private should be moved. ~ ejt
#define ArraySize(x) (sizeof((x)) / sizeof((x)[0]))

#define PRIVATE_UNIT 1
#define PRIVATE_ITEM 3

struct Private {
  DWORD dwPrivateType;
};

struct Variables {
  int nChickenHP;
  int nChickenMP;
  DWORD dwInjectTime;
  DWORD dwGameTime;
  BOOL bDontCatchNextMsg;
  BOOL bClickAction;
  BOOL bNeedShutdown;
  BOOL bUseGamePrint;
  BOOL bShutdownFromDllMain;
  BOOL bChangedAct;
  BOOL bGameLoopEntered;
  DWORD dwGameThreadId;
  DWORD dwLocale;

  DWORD dwMaxGameTime;
  DWORD dwGameTimeout;
  BOOL bTakeScreenshot;
  BOOL bQuitOnError;
  BOOL bQuitOnHostile;
  BOOL bStartAtMenu;
  BOOL bActive;
  BOOL bBlockKeys;
  BOOL bBlockMouse;
  BOOL bDisableCache;
  BOOL bUseProfileScript;
  bool bLoadedWithCGuard;
  BOOL bLogConsole;
  BOOL bEnableUnsupported;
  BOOL bForwardMessageBox;
  BOOL bUseRawCDKey;
  BOOL bQuitting;
  BOOL bCacheFix;
  BOOL bMulti;
  BOOL bSleepy;
  BOOL bReduceFTJ;
  int dwMemUsage;
  int dwConsoleFont;
  HANDLE eventSignal;
  Module* pModule;
  HMODULE hModule;
  HWND hHandle;

  std::filesystem::path working_dir;
  std::filesystem::path log_dir;
  std::filesystem::path script_dir;
  wchar_t szProfile[256];
  wchar_t szStarter[_MAX_FNAME];
  wchar_t szConsole[_MAX_FNAME];
  wchar_t szDefault[_MAX_FNAME];
  char szHosts[256];
  char szClassic[30];
  char szLod[30];
  wchar_t szTitle[256];
  wchar_t szCommandLine[256];

  WNDPROC oldWNDPROC;
  HHOOK hMouseHook;
  HHOOK hKeybHook;

  UINT_PTR uTimer;
  long SectionCount;

  std::queue<std::wstring> qPrintBuffer;
  std::map<unsigned __int32, CellFile*> mCachedCellFiles;
  std::vector<std::pair<DWORD, DWORD>> vUnitList;
  // std::list<Event*> EventList;
  CRITICAL_SECTION cEventSection;
  CRITICAL_SECTION cRoomSection;
  CRITICAL_SECTION cMiscSection;
  CRITICAL_SECTION cScreenhookSection;
  CRITICAL_SECTION cPrintSection;
  CRITICAL_SECTION cTextHookSection;
  CRITICAL_SECTION cImageHookSection;
  CRITICAL_SECTION cBoxHookSection;
  CRITICAL_SECTION cFrameHookSection;
  CRITICAL_SECTION cLineHookSection;
  CRITICAL_SECTION cFlushCacheSection;
  CRITICAL_SECTION cConsoleSection;
  CRITICAL_SECTION cGameLoopSection;
  CRITICAL_SECTION cUnitListSection;
  CRITICAL_SECTION cFileSection;

  DWORD dwSelectedUnitId;
  DWORD dwSelectedUnitType;
  POINT pMouseCoords;
};

extern Variables Vars;
