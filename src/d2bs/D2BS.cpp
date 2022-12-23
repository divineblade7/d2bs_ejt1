// Diablo II Botting System Core

#include "D2BS.h"

#include "d2bs/core/Core.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/handlers/D2Handlers.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/utils/CommandLine.h"
#include "d2bs/utils/Console.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/utils/Offset.h"
#include "d2bs/utils/dde.h"

#include <fcntl.h>
#include <io.h>
#include <shlwapi.h>

#ifdef _MSVC_DEBUG
#include "utils/D2Loader.h"
#endif

bool __fastcall UpdatePlayerGid(Script* script, void*, uint) {
  script->UpdatePlayerGid();
  return true;
}

static HANDLE main_thread_handle = INVALID_HANDLE_VALUE;
static HANDLE hEventThread = INVALID_HANDLE_VALUE;

DWORD WINAPI MainThread(LPVOID) {
  std::string arg_val;
  bool beginStarter = true;
  bool bInGame = false;

  if (!InitHooks()) {
    Log(L"D2BS Engine startup failed. %s", Vars.szCommandLine);
    Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup failed!");
    return FALSE;
  }

  Vars.bUseRawCDKey = FALSE;  // this can be set inside Variables instead to default to 'false'

  // This can be moved into a separate function
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

void Setup(HINSTANCE hDll, LPVOID lpReserved) {
  DisableThreadLibraryCalls(hDll);

  if (lpReserved) {
    Vars.pModule = (Module*)lpReserved;
    Vars.working_dir = Vars.pModule->szPath;
    Vars.bLoadedWithCGuard = true;
  } else {
    Vars.hModule = hDll;
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(hDll, path, MAX_PATH);
    Vars.working_dir = path;
    Vars.working_dir.remove_filename().make_preferred();
    Vars.bLoadedWithCGuard = false;
  }

  Vars.log_dir = Vars.working_dir / "logs";
  if (!std::filesystem::exists(Vars.log_dir)) {
    std::filesystem::create_directory(Vars.log_dir);
  }

  InitCommandLine();
  InitSettings();

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
}

BOOL Startup(void) {
  InitializeCriticalSection(&Vars.cEventSection);
  InitializeCriticalSection(&Vars.cPrintSection);
  InitializeCriticalSection(&Vars.cBoxHookSection);
  InitializeCriticalSection(&Vars.cFrameHookSection);
  InitializeCriticalSection(&Vars.cLineHookSection);
  InitializeCriticalSection(&Vars.cImageHookSection);
  InitializeCriticalSection(&Vars.cTextHookSection);
  InitializeCriticalSection(&Vars.cFlushCacheSection);
  InitializeCriticalSection(&Vars.cConsoleSection);
  InitializeCriticalSection(&Vars.cGameLoopSection);
  InitializeCriticalSection(&Vars.cFileSection);

  Vars.bNeedShutdown = TRUE;
  Vars.bChangedAct = FALSE;
  Vars.bGameLoopEntered = FALSE;
  Vars.SectionCount = 0;

  // MessageBox(NULL, "qwe", "qwe", MB_OK);
  Genhook::Initialize();
  DefineOffsets();
  InstallPatches();
  InstallConditional();
  CreateDdeServer();

  main_thread_handle = CreateThread(NULL, NULL, MainThread, NULL, NULL, NULL);
  if (!main_thread_handle) {
    return FALSE;
  }

  return TRUE;
}

void Shutdown(bool await_thread) {
  if (!Vars.bNeedShutdown) {
    return;
  }

  Vars.bActive = FALSE;
  if (await_thread) {
    WaitForSingleObject(main_thread_handle, INFINITE);
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
  DeleteCriticalSection(&Vars.cConsoleSection);
  DeleteCriticalSection(&Vars.cGameLoopSection);
  DeleteCriticalSection(&Vars.cFileSection);

  Log(L"D2BS Shutdown complete.");
  Vars.bNeedShutdown = false;
}

BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      Setup(hDll, lpReserved);
      return Startup();
    case DLL_PROCESS_DETACH:
      if (Vars.bNeedShutdown) {
        Shutdown();
      }
      break;
  }

  return TRUE;
}
