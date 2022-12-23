// Diablo II Botting System Core

#include "D2BS.h"

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

DWORD __stdcall thread_entry(void*) {
  std::string arg_val;
  bool beginStarter = true;
  bool bInGame = false;

  if (!InitHooks()) {
    Log(L"D2BS Engine startup failed. %s", GetCommandLineW());
    Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup failed!");
    return FALSE;
  }

  Vars.bUseRawCDKey = FALSE;  // can this can be set inside Variables instead to default to 'false'?

  sEngine->parse_commandline_args();

  Log(L"D2BS Engine startup complete. %s", L"" D2BS_VERSION);
  Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup complete!");

  while (Vars.bActive) {
    switch (ClientState()) {
      case ClientStateInGame: {
        if (bInGame) {
          if ((Vars.dwMaxGameTime && Vars.dwGameTime && (GetTickCount() - Vars.dwGameTime) > Vars.dwMaxGameTime) ||
              (!D2COMMON_IsTownByLevelNo(GetPlayerArea()) &&
                   (Vars.nChickenHP && Vars.nChickenHP >= GetUnitHP(D2CLIENT_GetPlayerUnit())) ||
               (Vars.nChickenMP && Vars.nChickenMP >= GetUnitMP(D2CLIENT_GetPlayerUnit()))))
            D2CLIENT_ExitGame();
        } else {
          Vars.dwGameTime = GetTickCount();

          Sleep(500);

          D2CLIENT_InitInventory();
          sScriptEngine->ForEachScript(UpdatePlayerGid, NULL, 0);
          sScriptEngine->UpdateConsole();
          Vars.bQuitting = false;
          GameJoined();

          bInGame = true;
        }
        break;
      }
      case ClientStateMenu: {
        while (Vars.bUseProfileScript) {
          Sleep(100);
        }
        MenuEntered(beginStarter);
        beginStarter = false;
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
    Sleep(50);
  }

  sScriptEngine->Shutdown();

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

  Vars.working_dir = root_dir_; // DEPRECATED
  logs_dir_ = root_dir_ / "logs";
  Vars.log_dir = logs_dir_;  // DEPRECATED
  settings_file_ = root_dir_ / "d2bs.ini";

  // create log directory if it does not exist
  if (!std::filesystem::exists(logs_dir_)) {
    std::filesystem::create_directory(logs_dir_);
  }
}

void D2BS::init_settings() {
  wchar_t scriptPath[_MAX_PATH], defaultStarter[_MAX_FNAME], defaultGame[_MAX_FNAME], defaultConsole[_MAX_FNAME],
      hosts[256], debug[6], quitOnHostile[6], quitOnError[6], startAtMenu[6],
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
