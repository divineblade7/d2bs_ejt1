#include "d2bs/new_core/application.h"

#include "d2bs/core/Control.h"
#include "d2bs/core/Core.h"
#include "d2bs/core/ScreenHook.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/handlers/D2Handlers.h"
#include "d2bs/new_util/localization.h"
#include "d2bs/utils/CommandLine.h"
#include "d2bs/utils/Console.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/utils/Offset.h"
#include "d2bs/variables.h"
#include "d2bs/version.h"

static WNDPROC orig_wndproc_ = nullptr;

LONG WINAPI wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  COPYDATASTRUCT* pCopy;
  switch (msg) {
    case WM_COPYDATA:
      pCopy = (COPYDATASTRUCT*)lparam;

      if (pCopy) {
        auto lpwData = d2bs::util::ansi_to_wide(static_cast<const char*>(pCopy->lpData));
        if (pCopy->dwData == 0x1337)  // 0x1337 = Execute Script
        {
          while (!Vars.bActive || (sScriptEngine->state() != Running)) {
            Sleep(100);
          }
          sScriptEngine->RunCommand(lpwData.c_str());
        } else if (pCopy->dwData == 0x31337) {  // 0x31337 = Set Profile
          if (Vars.settings.set_profile(lpwData.c_str())) {
            Print(L"\u00FFc2D2BS\u00FFc0 :: Switched to profile %s", lpwData.c_str());
          } else {
            Print(L"\u00FFc2D2BS\u00FFc0 :: Profile %s not found", lpwData.c_str());
          }
        } else {
          FireCopyDataEvent(pCopy->dwData, lpwData.data());
        }
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
        return (LONG)CallWindowProcA(orig_wndproc_, hwnd, msg, wparam, lparam);
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

          return (LONG)CallWindowProcA(orig_wndproc_, hwnd, msg, wparam, lparam);
        }
      } else if (key == VK_ESCAPE && sConsole->IsVisible()) {
        if (isDown && !isRepeat) {
          sConsole->Hide();
          return 1;
        }
        return (LONG)CallWindowProcA(orig_wndproc_, hwnd, msg, wparam, lparam);
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
            return (LONG)CallWindowProcA(orig_wndproc_, hwnd, msg, wparam, lparam);
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

  return (LONG)CallWindowProcA(orig_wndproc_, hwnd, msg, wparam, lparam);
}
namespace d2bs {

Application::Application() {
  Vars.settings.load(Vars.working_dir / "d2bs.ini");

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
  InitializeCriticalSection(&Vars.cBoxHookSection);
  InitializeCriticalSection(&Vars.cFrameHookSection);
  InitializeCriticalSection(&Vars.cLineHookSection);
  InitializeCriticalSection(&Vars.cImageHookSection);
  InitializeCriticalSection(&Vars.cTextHookSection);
  InitializeCriticalSection(&Vars.cFlushCacheSection);
  InitializeCriticalSection(&Vars.cGameLoopSection);
  InitializeCriticalSection(&Vars.cFileSection);

  Vars.bChangedAct = FALSE;
  Vars.bGameLoopEntered = FALSE;
  Vars.SectionCount = 0;

  Genhook::Initialize();
  DefineOffsets();
  InstallPatches();
  InstallConditional();

#ifdef DEBUG
  SetThreadDescription(GetCurrentThread(), L"MainThread");
#endif

  if (!init_hooks()) {
    Log(L"D2BS Engine startup failed. %s", GetCommandLineW());
    Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup failed!");
    throw std::exception("Failed to startup");
  }

  if (!sScriptEngine->init()) {
    Log(L"Failed to startup script engine");
    Print(L"\u00FFc2D2BS\u00FFc0 :: Script engine startup failed!");
    throw std::exception("Failed to startup");
  }

  if (ClientState() == ClientStateMenu && Vars.settings.bStartAtMenu) {
    clickControl(*p_D2WIN_FirstControl);
  }

  *p_D2CLIENT_Lang = D2CLIENT_GetGameLanguageCode();
  Vars.dwLocale = *p_D2CLIENT_Lang;

  parse_commandline_args();

  Log(L"D2BS Engine startup complete. %s", L"" D2BS_VERSION);
  Print(L"\u00FFc2D2BS\u00FFc0 :: Engine startup complete!");
}

Application::~Application() {
  sScriptEngine->shutdown();

  SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)orig_wndproc_);

  RemovePatches();
  Genhook::Destroy();

  KillTimer(D2GFX_GetHwnd(), Vars.uTimer);

  DeleteCriticalSection(&Vars.cBoxHookSection);
  DeleteCriticalSection(&Vars.cFrameHookSection);
  DeleteCriticalSection(&Vars.cLineHookSection);
  DeleteCriticalSection(&Vars.cImageHookSection);
  DeleteCriticalSection(&Vars.cTextHookSection);
  DeleteCriticalSection(&Vars.cFlushCacheSection);
  DeleteCriticalSection(&Vars.cGameLoopSection);
  DeleteCriticalSection(&Vars.cFileSection);

  Log(L"D2BS Shutdown complete.");
}

void Application::run() {
  while (Vars.bActive) {
    static bool bInGame = false;

    switch (ClientState()) {
      case ClientStateInGame: {
        if (!bInGame) {
          Vars.settings.dwGameTime = GetTickCount();

          // Is this Zzzz necessary? ~ ejt
          Sleep(500);

          D2CLIENT_InitInventory();
          sScriptEngine->for_each([](Script* script) {
            script->UpdatePlayerGid();
            return true;
          });
          Vars.bQuitting = false;

          handle_enter_game();

          bInGame = true;
        }

        if ((Vars.settings.dwMaxGameTime && Vars.settings.dwGameTime &&
             (GetTickCount() - Vars.settings.dwGameTime) > Vars.settings.dwMaxGameTime) ||
            (!D2COMMON_IsTownByLevelNo(GetPlayerArea()) &&
                 (Vars.nChickenHP && Vars.nChickenHP >= GetUnitHP(D2CLIENT_GetPlayerUnit())) ||
             (Vars.nChickenMP && Vars.nChickenMP >= GetUnitMP(D2CLIENT_GetPlayerUnit())))) {
          D2CLIENT_ExitGame();
        }
        break;
      }
      case ClientStateMenu: {
        // i think this variable is to wait for a profile to start using SwitchToProfile? ~ ejt
        while (Vars.settings.bUseProfileScript) {
          Sleep(100);
        }

        static bool first_menu_call = true;
        if (first_menu_call) {
          handle_enter_game();
          first_menu_call = false;
        }

        if (bInGame) {
          Vars.settings.dwGameTime = NULL;
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
}

void Application::parse_commandline_args() {
  CommandLine cmdline(GetCommandLineA());
  for (const auto& [arg, val] : cmdline.args()) {
    if (arg == "-title") {
      auto text = d2bs::util::ansi_to_wide(val);
      wcsncat_s(Vars.szTitle, text.c_str(), text.length());
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
      auto profile = d2bs::util::ansi_to_wide(val);
      if (Vars.settings.set_profile(profile))
        Print(L"\u00FFc2D2BS\u00FFc0 :: Switched to profile %s (-profile)", profile.c_str());
      else
        Print(L"\u00FFc2D2BS\u00FFc0 :: Profile %s not found (-profile)", profile.c_str());
    }
  }
}

bool Application::init_hooks() {
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

void Application::handle_enter_game() {
  if (!Vars.settings.bUseProfileScript) {
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

}  // namespace d2bs
