// Diablo II Botting System Core

#include "engine.h"

#include "d2bs/core/Control.h"
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

namespace d2bs {

// forward-declare `thread_entry` and `wndproc` so that is can be defined in the proper
// order accoring to how it is declared inside `D2BS`
DWORD __stdcall thread_entry(void* param);
LONG WINAPI wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

bool Engine::startup(HMODULE mod) {
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

#if DEBUG
  SetUnhandledExceptionFilter(ExceptionHandler);
#endif

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
  dde_.init();

  thread_handle_ = CreateThread(NULL, NULL, thread_entry, NULL, NULL, NULL);
  if (!thread_handle_) {
    return FALSE;
  }

  return TRUE;
}

void Engine::shutdown(bool await_thread) {
  if (!initialized_) {
    return;
  }

  Vars.bActive = FALSE;
  if (await_thread) {
    WaitForSingleObject(thread_handle_, INFINITE);
  }

  SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)orig_wndproc_);

  RemovePatches();
  Genhook::Destroy();
  dde_.shutdown();

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

void Engine::update() {
  static bool bInGame = false;

  switch (ClientState()) {
    case ClientStateInGame: {
      if (!bInGame) {
        Vars.dwGameTime = GetTickCount();

        // Is this Zzzz necessary? ~ ejt
        Sleep(500);

        D2CLIENT_InitInventory();
        script_engine_.for_each([](Script* script) {
          script->UpdatePlayerGid();
          return true;
        });
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

      if (first_menu_call_) {
        on_game_enter();
        first_menu_call_ = false;
      }

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

void Engine::on_game_enter() {
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

void Engine::on_game_draw() {
  if (Vars.bActive && ClientState() == ClientStateInGame) {
    FlushPrint();
    Genhook::DrawAll(IG);
    DrawLogo();
    sConsole->Draw();
  }

  if (Vars.bTakeScreenshot) {
    Vars.bTakeScreenshot = false;
    D2WIN_TakeScreenshot();
  }

  if (Vars.SectionCount) {
    if (Vars.bGameLoopEntered)
      LeaveCriticalSection(&Vars.cGameLoopSection);
    else
      Vars.bGameLoopEntered = true;
    Sleep(0);
    EnterCriticalSection(&Vars.cGameLoopSection);
  } else
    Sleep(10);
}

void Engine::on_menu_draw() {
  D2WIN_DrawSprites();
  if (Vars.bActive && ClientState() == ClientStateMenu) {
    FlushPrint();
    Genhook::DrawAll(OOG);
    DrawLogo();
    sConsole->Draw();
  }
  if (Vars.bTakeScreenshot) {
    Vars.bTakeScreenshot = false;
    D2WIN_TakeScreenshot();
  }
  Sleep(10);
}

void Engine::run_chicken() {
  if ((Vars.dwMaxGameTime && Vars.dwGameTime && (GetTickCount() - Vars.dwGameTime) > Vars.dwMaxGameTime) ||
      (!D2COMMON_IsTownByLevelNo(GetPlayerArea()) &&
           (Vars.nChickenHP && Vars.nChickenHP >= GetUnitHP(D2CLIENT_GetPlayerUnit())) ||
       (Vars.nChickenMP && Vars.nChickenMP >= GetUnitMP(D2CLIENT_GetPlayerUnit())))) {
    D2CLIENT_ExitGame();
  }
}

DWORD __stdcall thread_entry(void*) {
#ifdef DEBUG
  SetThreadDescription(GetCurrentThread(), L"MainThread");
#endif

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

LONG WINAPI wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  Engine* engine = sEngine;

  COPYDATASTRUCT* pCopy;
  switch (msg) {
    case WM_COPYDATA:
      pCopy = (COPYDATASTRUCT*)lparam;

      if (pCopy) {
        wchar_t* lpwData = AnsiToUnicode((const char*)pCopy->lpData);
        if (pCopy->dwData == 0x1337)  // 0x1337 = Execute Script
        {
          while (!Vars.bActive || (sScriptEngine->GetState() != Running)) {
            Sleep(100);
          }
          sScriptEngine->RunCommand(lpwData);
        } else if (pCopy->dwData == 0x31337)  // 0x31337 = Set Profile
          if (SwitchToProfile(lpwData))
            Print(L"\u00FFc2D2BS\u00FFc0 :: Switched to profile %s", lpwData);
          else
            Print(L"\u00FFc2D2BS\u00FFc0 :: Profile %s not found", lpwData);
        else
          FireCopyDataEvent(pCopy->dwData, lpwData);
        delete[] lpwData;
      }

      return TRUE;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
      // This is straight copy-pasted from previous KeyPress function, needs a cleanup
      short key = LOWORD(wparam);
      WORD repeatCount = LOWORD(lparam);
      bool altState = (HIWORD(lparam) & KF_ALTDOWN) == KF_ALTDOWN;
      bool previousState = (HIWORD(lparam) & KF_REPEAT) == KF_REPEAT;
      bool transitionState = (HIWORD(lparam) & KF_UP) == KF_UP;
      bool isRepeat = !transitionState && repeatCount != 1;
      bool isDown = !(previousState && transitionState);
      bool isUp = previousState && transitionState;

      bool gameState = ClientState() == ClientStateInGame;
      bool chatBoxOpen = gameState ? !!D2CLIENT_GetUIState(UI_CHAT_CONSOLE) : false;
      bool escMenuOpen = gameState ? !!D2CLIENT_GetUIState(UI_ESCMENU_MAIN) : false;

      if (altState && wparam == VK_F4) {
        return (LONG)CallWindowProcA(engine->orig_wndproc_, hwnd, msg, wparam, lparam);
      }

      if (Vars.bBlockKeys) {
        return 1;
      }

      if (key == VK_HOME && !(chatBoxOpen || escMenuOpen)) {
        if (isDown && !isRepeat) {
          if (!altState)
            sConsole->ToggleBuffer();
          else
            sConsole->TogglePrompt();

          return (LONG)CallWindowProcA(engine->orig_wndproc_, hwnd, msg, wparam, lparam);
        }
      } else if (key == VK_ESCAPE && sConsole->IsVisible()) {
        if (isDown && !isRepeat) {
          sConsole->Hide();
          return 1;
        }
        return (LONG)CallWindowProcA(engine->orig_wndproc_, hwnd, msg, wparam, lparam);
      } else if (sConsole->IsEnabled()) {
        BYTE layout[256] = {0};
        WORD out[2] = {0};
        switch (key) {
          case VK_TAB:
            if (isUp)
              for (int i = 0; i < 5; i++) sConsole->AddKey(' ');
            break;
          case VK_RETURN:
            if (isUp && !isRepeat && !escMenuOpen) sConsole->ExecuteCommand();
            break;
          case VK_BACK:
            if (isDown) sConsole->RemoveLastKey();
            break;
          case VK_UP:
            if (isUp && !isRepeat) sConsole->PrevCommand();
            break;
          case VK_DOWN:
            if (isUp && !isRepeat) sConsole->NextCommand();
            break;
          case VK_NEXT:
            if (isDown) sConsole->ScrollDown();
            break;
          case VK_PRIOR:
            if (isDown) sConsole->ScrollUp();
            break;
          case VK_MENU:  // alt
            // Send the alt to the scripts to fix sticky alt. There may be a better way.
            FireKeyDownUpEvent(wparam, isUp);
            return (LONG)CallWindowProcA(engine->orig_wndproc_, hwnd, msg, wparam, lparam);
            break;
          default:
            if (isDown) {
              if (GetKeyboardState(layout) && ToAscii(wparam, (lparam & 0xFF0000), layout, out, 0) != 0) {
                for (int i = 0; i < repeatCount; i++) sConsole->AddKey(out[0]);
              }
            }
            break;
        }
        return 1;
      } else if (!isRepeat && !(chatBoxOpen || escMenuOpen)) {
        if (FireKeyDownUpEvent(key, isUp)) return 1;
      }

      break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK: {
      int32_t button = 0;
      if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
      if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
      if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
      if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) button = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) ? 3 : 4;

      POINT pt = {static_cast<LONG>(LOWORD(lparam)), static_cast<LONG>(HIWORD(lparam))};
      Vars.pMouseCoords = pt;
      if (Vars.bBlockMouse) {
        break;
      }

      HookClickHelper helper = {-1, {pt.x, pt.y}};
      FireMouseClickEvent(button, pt, false);
      helper.button = button;
      if (Genhook::ForEachVisibleHook(ClickHook, &helper, 1)) {
        // a positive result means a function used the message, swallow it by returning here.
        return 0;
      }
      break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
      int32_t button = 0;
      if (msg == WM_LBUTTONUP) button = 0;
      if (msg == WM_RBUTTONUP) button = 1;
      if (msg == WM_MBUTTONUP) button = 2;
      if (msg == WM_XBUTTONUP) button = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) ? 3 : 4;

      POINT pt = {static_cast<LONG>(LOWORD(lparam)), static_cast<LONG>(HIWORD(lparam))};
      Vars.pMouseCoords = pt;
      if (Vars.bBlockMouse) {
        break;
      }

      FireMouseClickEvent(button, pt, true);
      break;
    }

    case WM_MOUSEMOVE:
      POINT pt = {static_cast<LONG>(LOWORD(lparam)), static_cast<LONG>(HIWORD(lparam))};
      Vars.pMouseCoords = pt;
      if (Vars.bBlockMouse) {
        break;
      }

      // would be nice to enable these events but they bog down too much
      FireMouseMoveEvent(pt);
      // Genhook::ForEachVisibleHook(HoverHook, &helper, 1);
      break;
  }

  return (LONG)CallWindowProcA(engine->orig_wndproc_, hwnd, msg, wparam, lparam);
}

void Engine::parse_commandline_args() {
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
        Print(L"\u00FFc2D2BS\u00FFc0 :: Switched to profile %s (-profile)", profile);
      else
        Print(L"\u00FFc2D2BS\u00FFc0 :: Profile %s not found (-profile)", profile);
      delete[] profile;  // ugh...
    }
  }
}

void Engine::init_paths(HMODULE mod) {
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

void Engine::init_settings() {
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
    Vars.dwMemUsage = 500;
  }
  Vars.dwMemUsage *= 1024 * 1024;
  orig_wndproc_ = nullptr;
}

bool Engine::init_hooks() {
  // Is this sleep necessary? ~ ejt
  Sleep(50);

  while (!orig_wndproc_) {
    orig_wndproc_ = (WNDPROC)SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)wndproc);
  }

  Vars.uTimer = SetTimer(D2GFX_GetHwnd(), 1, 0, TimerProc);

  // is this variable necessary? ~ ejt
  Vars.bActive = TRUE;
  return true;
}

}  // namespace d2bs
