// Diablo II Botting System Core

#include "D2BS.h"

#include "d2bs/core/Control.h"
#include "d2bs/core/Core.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/handlers/D2Handlers.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/utils/CommandLine.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/utils/Offset.h"
#include "d2bs/utils/dde.h"

#include <fcntl.h>
#include <io.h>
#include <shlwapi.h>

#ifdef _MSVC_DEBUG
#include "utils/D2Loader.h"
#endif

// deprecate this function, some hack workaround implemented by someone long ago...
bool __fastcall UpdatePlayerGid(Script* script, void*, uint) {
  script->UpdatePlayerGid();
  return true;
}

// forward-declare `thread_entry` so that is can be defined in the proper
// order accoring to how it is declared inside `D2BS`
DWORD __stdcall thread_entry(void* param);

bool D2BS::startup(HMODULE mod) {
  Vars.hModule = mod;

  init_paths(mod);
  init_settings();

#if 0
  char errlog[516] = "";
  sprintf_s(errlog, 516, "%sd2bs.log", Vars.szPath);
  AllocConsole();
  int handle = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
  FILE* f = _fdopen(handle, "wt");
  *stderr = *f;
  setvbuf(stderr, NULL, _IONBF, 0);
  freopen_s(&f, errlog, "a+t", f);
#endif

  SetUnhandledExceptionFilter(ExceptionHandler);

  InitializeCriticalSection(&Vars.cEventSection);
  InitializeCriticalSection(&Vars.cPrintSection);
  InitializeCriticalSection(&Vars.cBoxHookSection);
  InitializeCriticalSection(&Vars.cFrameHookSection);
  InitializeCriticalSection(&Vars.cLineHookSection);
  InitializeCriticalSection(&Vars.cImageHookSection);
  InitializeCriticalSection(&Vars.cTextHookSection);
  InitializeCriticalSection(&Vars.cFlushCacheSection);
  InitializeCriticalSection(&Vars.cGameLoopSection);
  InitializeCriticalSection(&Vars.cFileSection);

  initialized_ = true;
  Vars.bChangedAct = FALSE;
  Vars.bGameLoopEntered = FALSE;
  Vars.SectionCount = 0;

  Genhook::Initialize();
  DefineOffsets();
  InstallPatches();
  InstallConditional();
  CreateDdeServer();

  thread_handle_ = CreateThread(NULL, NULL, thread_entry, NULL, NULL, NULL);
  if (!thread_handle_) {
    return FALSE;
  }

  return TRUE;
}

void D2BS::shutdown(bool await_thread) {
  if (!initialized_) {
    return;
  }

  Vars.bActive = FALSE;
  if (await_thread) {
    WaitForSingleObject(thread_handle_, INFINITE);
  }

  SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)Vars.oldWNDPROC);

  RemovePatches();
  Genhook::Destroy();
  ShutdownDdeServer();

  KillTimer(D2GFX_GetHwnd(), Vars.uTimer);

  UnhookWindowsHookEx(Vars.hMouseHook);
  UnhookWindowsHookEx(Vars.hKeybHook);

  DeleteCriticalSection(&Vars.cPrintSection);
  DeleteCriticalSection(&Vars.cBoxHookSection);
  DeleteCriticalSection(&Vars.cFrameHookSection);
  DeleteCriticalSection(&Vars.cLineHookSection);
  DeleteCriticalSection(&Vars.cImageHookSection);
  DeleteCriticalSection(&Vars.cTextHookSection);
  DeleteCriticalSection(&Vars.cFlushCacheSection);
  DeleteCriticalSection(&Vars.cGameLoopSection);
  DeleteCriticalSection(&Vars.cFileSection);

  Log(L"D2BS Shutdown complete.");
  initialized_ = false;
}

void D2BS::update() {
  static bool bInGame = false;

  switch (ClientState()) {
    case ClientStateInGame: {
      if (!bInGame) {
        Vars.dwGameTime = GetTickCount();

        // Is this Zzzz necessary? ~ ejt
        Sleep(500);

        D2CLIENT_InitInventory();
        script_engine_.ForEachScript(UpdatePlayerGid, NULL, 0);
        script_engine_.UpdateConsole();
        Vars.bQuitting = false;
        on_game_enter();

        bInGame = true;
      }

      run_chicken();
      break;
    }
    case ClientStateMenu: {
      // i think this variable is to wait for a profile to start using SwitchToProfile? ~ ejt
      while (Vars.bUseProfileScript) {
        Sleep(100);
      }

      on_menu_enter();
      if (bInGame) {
        Vars.dwGameTime = NULL;
        bInGame = false;
      }
      break;
    }
    case ClientStateBusy:
    case ClientStateNull:
      break;
  }
}

void D2BS::on_game_enter() {
  if (!Vars.bUseProfileScript) {
    const wchar_t* starter = GetStarterScriptName();
    if (starter != NULL) {
      Print(L"\u00FFc2D2BS\u00FFc0 :: Starting %s", starter);
      if (StartScript(starter, GetStarterScriptState()))
        Print(L"\u00FFc2D2BS\u00FFc0 :: %s running.", starter);
      else
        Print(L"\u00FFc2D2BS\u00FFc0 :: Failed to start %s!", starter);
    }
  }
}

void D2BS::on_menu_enter() {
  if (first_menu_call_ && !Vars.bUseProfileScript) {
    const wchar_t* starter = GetStarterScriptName();
    if (starter != NULL) {
      Print(L"\u00FFc2D2BS\u00FFc0 :: Starting %s", starter);
      if (StartScript(starter, GetStarterScriptState())) {
        Print(L"\u00FFc2D2BS\u00FFc0 :: %s running.", starter);
      } else {
        Print(L"\u00FFc2D2BS\u00FFc0 :: Failed to start %s!", starter);
      }
    }
    first_menu_call_ = false;
  }
}

void D2BS::run_chicken() {
  if ((Vars.dwMaxGameTime && Vars.dwGameTime && (GetTickCount() - Vars.dwGameTime) > Vars.dwMaxGameTime) ||
      (!D2COMMON_IsTownByLevelNo(GetPlayerArea()) &&
           (Vars.nChickenHP && Vars.nChickenHP >= GetUnitHP(D2CLIENT_GetPlayerUnit())) ||
       (Vars.nChickenMP && Vars.nChickenMP >= GetUnitMP(D2CLIENT_GetPlayerUnit())))) {
    D2CLIENT_ExitGame();
  }
}

DWORD __stdcall thread_entry(void*) {
  auto engine = sEngine;

  // TODO REMOVE: Temporary during refactoring of ScriptEngine
  sScriptEngine = engine->script_engine();

  if (!engine->init_hooks()) {
    Log(L"D2BS Engine startup failed. %s", GetCommandLineW());
    Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup failed!");
    return FALSE;
  }

  if (!engine->script_engine()->Startup()) {
    Log(L"Failed to startup script engine");
    Print(L"\u00FFc2D2BS\u00FFc0 :: Script engine startup failed!");
    return false;
  }

  if (ClientState() == ClientStateMenu && Vars.bStartAtMenu) {
    clickControl(*p_D2WIN_FirstControl);
  }

  *p_D2CLIENT_Lang = D2CLIENT_GetGameLanguageCode();
  Vars.dwLocale = *p_D2CLIENT_Lang;

  engine->parse_commandline_args();

  Log(L"D2BS Engine startup complete. %s", L"" D2BS_VERSION);
  Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup complete!");

  while (Vars.bActive) {
    engine->update();
    Sleep(50);
  }

  engine->script_engine()->Shutdown();

  return NULL;
}

void D2BS::parse_commandline_args() {
  CommandLine cmdline(GetCommandLineA());
  for (const auto& [arg, val] : cmdline.args()) {
    if (arg == "-title") {
      const wchar_t* text = AnsiToUnicode(val.c_str());
      int len = wcslen((wchar_t*)text);
      wcsncat_s(Vars.szTitle, (wchar_t*)text, len);
      delete[] text;  // ugh...
    } else if (arg == "-sleepy") {
      Vars.bSleepy = TRUE;
    } else if (arg == "-cachefix") {
      Vars.bCacheFix = TRUE;
    } else if (arg == "-multi") {
      Vars.bMulti = TRUE;
    } else if (arg == "-ftj") {
      Vars.bReduceFTJ = TRUE;
    } else if (arg == "-d2c") {
      Vars.bUseRawCDKey = TRUE;
      strncat_s(Vars.szClassic, val.c_str(), val.length());
    } else if (arg == "-d2x") {
      strncat_s(Vars.szLod, val.c_str(), val.length());
    } else if (arg == "-handle") {
      Vars.hHandle = (HWND)atoi(val.c_str());
    } else if (arg == "-mpq") {
      LoadMPQ(val.c_str());
    } else if (arg == "-profile") {
      const wchar_t* profile = AnsiToUnicode(val.c_str());
      if (SwitchToProfile(profile))
        Print(L"\u00FFc2D2BS\u00FFc0 :: Switched to profile %s", profile);
      else
        Print(L"\u00FFc2D2BS\u00FFc0 :: Profile %s not found", profile);
      delete[] profile;  // ugh...
    }
  }
}

void D2BS::init_paths(HMODULE mod) {
  // grab root directory from input module pointer
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(mod, path, MAX_PATH);
  root_dir_ = path;
  // remove filename from path and make preferred, '\\' instead of '/'
  root_dir_.remove_filename();

  Vars.working_dir = root_dir_;  // DEPRECATED
  logs_dir_ = root_dir_ / "logs";
  Vars.log_dir = logs_dir_;  // DEPRECATED
  settings_file_ = root_dir_ / "d2bs.ini";

  // create log directory if it does not exist
  if (!std::filesystem::exists(logs_dir_)) {
    std::filesystem::create_directory(logs_dir_);
  }
}

void D2BS::init_settings() {
  wchar_t scriptPath[_MAX_PATH], hosts[256], debug[6], quitOnHostile[6], quitOnError[6], startAtMenu[6],
      disableCache[6], memUsage[6], gamePrint[6], useProfilePath[6], logConsole[6], enableUnsupported[6],
      forwardMessageBox[6], consoleFont[6];
  int maxGameTime = 0;
  int gameTimeout = 0;

  auto fname = settings_file_.c_str();

  GetPrivateProfileStringW(L"settings", L"ScriptPath", L"scripts", scriptPath, _MAX_PATH, fname);
  GetPrivateProfileStringW(L"settings", L"DefaultConsoleScript", L"", Vars.szConsole, _MAX_FNAME, fname);
  GetPrivateProfileStringW(L"settings", L"DefaultGameScript", L"default.dbj", Vars.szDefault, _MAX_FNAME, fname);
  GetPrivateProfileStringW(L"settings", L"DefaultStarterScript", L"starter.dbj", Vars.szStarter, _MAX_FNAME, fname);
  GetPrivateProfileStringW(L"settings", L"Hosts", L"", hosts, 256, fname);
  maxGameTime = GetPrivateProfileIntW(L"settings", L"MaxGameTime", 0, fname);
  GetPrivateProfileStringW(L"settings", L"Debug", L"false", debug, 6, fname);
  GetPrivateProfileStringW(L"settings", L"QuitOnHostile", L"false", quitOnHostile, 6, fname);
  GetPrivateProfileStringW(L"settings", L"QuitOnError", L"false", quitOnError, 6, fname);
  GetPrivateProfileStringW(L"settings", L"StartAtMenu", L"true", startAtMenu, 6, fname);
  GetPrivateProfileStringW(L"settings", L"DisableCache", L"true", disableCache, 6, fname);
  GetPrivateProfileStringW(L"settings", L"MemoryLimit", L"100", memUsage, 6, fname);
  GetPrivateProfileStringW(L"settings", L"UseGamePrint", L"false", gamePrint, 6, fname);
  gameTimeout = GetPrivateProfileIntW(L"settings", L"GameReadyTimeout", 5, fname);
  GetPrivateProfileStringW(L"settings", L"UseProfileScript", L"false", useProfilePath, 6, fname);
  GetPrivateProfileStringW(L"settings", L"LogConsoleOutput", L"false", logConsole, 6, fname);
  GetPrivateProfileStringW(L"settings", L"EnableUnsupported", L"false", enableUnsupported, 6, fname);
  GetPrivateProfileStringW(L"settings", L"ForwardMessageBox", L"false", forwardMessageBox, 6, fname);
  GetPrivateProfileStringW(L"settings", L"ConsoleFont", L"0", consoleFont, 6, fname);

  Vars.script_dir = Vars.working_dir / scriptPath;

  char* szHosts = UnicodeToAnsi(hosts);
  strcpy_s(Vars.szHosts, 256, szHosts);
  delete[] szHosts;

  Vars.dwGameTime = GetTickCount();
  Vars.dwMaxGameTime = abs(maxGameTime * 1000);
  Vars.dwGameTimeout = abs(gameTimeout * 1000);

  Vars.bQuitOnHostile = StringToBool(quitOnHostile);
  Vars.bQuitOnError = StringToBool(quitOnError);
  Vars.bStartAtMenu = StringToBool(startAtMenu);
  Vars.bDisableCache = StringToBool(disableCache);
  Vars.bUseGamePrint = StringToBool(gamePrint);
  Vars.bUseProfileScript = StringToBool(useProfilePath);
  Vars.bLogConsole = StringToBool(logConsole);
  Vars.bEnableUnsupported = StringToBool(enableUnsupported);
  Vars.bForwardMessageBox = StringToBool(forwardMessageBox);
  Vars.eventSignal = CreateEventA(nullptr, true, false, nullptr);
  Vars.dwMemUsage = abs(_wtoi(memUsage));
  Vars.dwConsoleFont = abs(_wtoi(consoleFont));
  if (Vars.dwMemUsage < 1) {
    Vars.dwMemUsage = 50;
  }
  Vars.dwMemUsage *= 1024 * 1024;
  Vars.oldWNDPROC = NULL;
}

bool D2BS::init_hooks() {
  // Is this sleep necessary? ~ ejt
  Sleep(50);

  while (!Vars.oldWNDPROC) {
    Vars.oldWNDPROC = (WNDPROC)SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)GameEventHandler);
  }

  Vars.uTimer = SetTimer(D2GFX_GetHwnd(), 1, 0, TimerProc);

  DWORD mainThread = GetWindowThreadProcessId(D2GFX_GetHwnd(), 0);
  // refactor this hook into the WndProc instead
  Vars.hKeybHook = SetWindowsHookEx(WH_KEYBOARD, KeyPress, NULL, mainThread);
  if (!Vars.hKeybHook) {
    MessageBox(0, "Failed to install keylogger!", "D2BS", 0);
    return false;
  }

  // refactor this hook into the WndProc instead
  Vars.hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseMove, NULL, mainThread);
  if (!Vars.hMouseHook) {
    MessageBox(0, "Failed to install mouselogger!", "D2BS", 0);
    return false;
  }

  // is this variable necessary? ~ ejt
  Vars.bActive = TRUE;
  return true;
}
