#include "d2bs/diablo/handlers/D2Handlers.h"

#include "d2bs/core/Control.h"
#include "d2bs/core/Core.h"
#include "d2bs/core/ScreenHook.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/handlers/D2NetHandlers.h"
#include "d2bs/engine.h"
#include "d2bs/script/Script.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/script/event.h"
#include "d2bs/utils/CommandLine.h"
#include "d2bs/utils/Console.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/utils/MapHeader.h"
#include "d2bs/utils/Offset.h"

#include <algorithm>
#include <string>
#include <vector>

DWORD __fastcall GameInput(wchar_t* wMsg) {
  bool send = true;

  if (Vars.bDontCatchNextMsg)
    Vars.bDontCatchNextMsg = false;
  else {
    send = !((wMsg[0] == L'.' && ProcessCommand(wMsg + 1, false)) || ChatInputEvent(wMsg));
  }

  return send ? 0 : -1;  // -1 means block, 0 means send
}

DWORD __fastcall ChannelInput(wchar_t* wMsg) {
  bool send = true;

  if (Vars.bDontCatchNextMsg)
    Vars.bDontCatchNextMsg = false;
  else {
    send = !((wMsg[0] == L'.' && ProcessCommand(wMsg + 1, false)) || ChatInputEvent(wMsg));
  }

  return send;  // false means ignore, true means send
}

DWORD __fastcall GamePacketReceived(BYTE* pPacket, DWORD dwSize) {
  switch (pPacket[0]) {
    case 0xAE:
      Log(L"Warden activity detected! Terminating Diablo to ensure your safety :)");
      TerminateProcess(GetCurrentProcess(), 0);
      break;
    case 0x15:
      return !GamePacketEvent(pPacket, dwSize) && ReassignPlayerHandler(pPacket, dwSize);
    case 0x26:
      return !GamePacketEvent(pPacket, dwSize) && ChatEventHandler(pPacket, dwSize);
    case 0x2A:
      return !GamePacketEvent(pPacket, dwSize) && NPCTransactionHandler(pPacket, dwSize);
    case 0x5A:
      return !GamePacketEvent(pPacket, dwSize) && EventMessagesHandler(pPacket, dwSize);
    case 0x18:
    case 0x95:
      return !GamePacketEvent(pPacket, dwSize) && HPMPUpdateHandler(pPacket, dwSize);
    case 0x9C:
    case 0x9D:
      return !GamePacketEvent(pPacket, dwSize) && ItemActionHandler(pPacket, dwSize);
    case 0xA7:
      return !GamePacketEvent(pPacket, dwSize) && DelayedStateHandler(pPacket, dwSize);
  }

  return !GamePacketEvent(pPacket, dwSize);
}

DWORD __fastcall GamePacketSent(BYTE* pPacket, DWORD dwSize) {
  return !GamePacketSentEvent(pPacket, dwSize);
}

LRESULT CALLBACK MouseMove(int code, WPARAM wParam, LPARAM lParam) {
  MOUSEHOOKSTRUCT* mouse = (MOUSEHOOKSTRUCT*)lParam;
  POINT pt = mouse->pt;
  ScreenToClient(mouse->hwnd, &pt);

  // filter out clicks on the window border
  if (code == HC_ACTION && (pt.x < 0 || pt.y < 0)) {
    return CallNextHookEx(NULL, code, wParam, lParam);
  }

  Vars.pMouseCoords = pt;
  if (Vars.bBlockMouse) {
    return 1;
  }

  if (code == HC_ACTION) {
    bool clicked = false;

    HookClickHelper helper = {-1, {pt.x, pt.y}};
    switch (wParam) {
      case WM_LBUTTONDOWN:
        MouseClickEvent(0, pt, false);
        helper.button = 0;
        if (Genhook::ForEachVisibleHook(ClickHook, &helper, 1)) {
          clicked = true;
        }
        break;
      case WM_LBUTTONUP:
        MouseClickEvent(0, pt, true);
        break;
      case WM_RBUTTONDOWN:
        MouseClickEvent(1, pt, false);
        helper.button = 1;
        if (Genhook::ForEachVisibleHook(ClickHook, &helper, 1)) {
          clicked = true;
        }
        break;
      case WM_RBUTTONUP:
        MouseClickEvent(1, pt, true);
        break;
      case WM_MBUTTONDOWN:
        MouseClickEvent(2, pt, false);
        helper.button = 2;
        if (Genhook::ForEachVisibleHook(ClickHook, &helper, 1)) {
          clicked = true;
        }
        break;
      case WM_MBUTTONUP:
        MouseClickEvent(2, pt, true);
        break;
      case WM_MOUSEMOVE:
        // would be nice to enable these events but they bog down too much
        MouseMoveEvent(pt);
        // Genhook::ForEachVisibleHook(HoverHook, &helper, 1);
        break;
    }

    return clicked ? 1 : CallNextHookEx(NULL, code, wParam, lParam);
  }

  return CallNextHookEx(NULL, code, wParam, lParam);
}

void FlushPrint() {
  if (!TryEnterCriticalSection(&Vars.cPrintSection)) return;

  if (Vars.qPrintBuffer.empty()) {
    LeaveCriticalSection(&Vars.cPrintSection);
    return;
  }

  std::queue<std::wstring> clean;
  std::swap(Vars.qPrintBuffer, clean);
  LeaveCriticalSection(&Vars.cPrintSection);

  while (!clean.empty()) {
    std::wstring str = clean.front();

    // Break into lines through \n.
    std::list<std::wstring> lines;
    std::wstring temp;
    std::wstringstream ss(str);

    if (Vars.bUseGamePrint && ClientState() == ClientStateInGame) {
      while (getline(ss, temp)) {
        SplitLines(temp.c_str(), sConsole->MaxWidth() - 100, L' ', lines);
        sConsole->AddLine(temp);
      }

      // Convert and send every line.
      for (std::list<std::wstring>::iterator it = lines.begin(); it != lines.end(); ++it) {
        D2CLIENT_PrintGameString((wchar_t*)it->c_str(), 0);
      }
      /*} else if (Vars.bUseGamePrint && ClientState() == ClientStateMenu && findControl(4, (const wchar_t*)NULL, -1,
         28, 410, 354, 298)) { while (getline(ss, temp)) SplitLines(temp, sConsole->MaxWidth() - 100, ' ', lines);
              // TODO: Double check this function, make sure it is working as intended.
              for (list<string>::iterator it = lines.begin(); it != lines.end(); ++it)
                  D2MULTI_PrintChannelText((char*)it->c_str(), 0);*/
    } else {
      while (getline(ss, temp)) sConsole->AddLine(temp);
    }

    clean.pop();
  }
}

void GameDraw(void) {
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

void GameDrawOOG(void) {
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

void SetMaxDiff(void) {
  if (D2CLIENT_GetDifficulty() == 1 && *p_D2CLIENT_ExpCharFlag) {
    BnetData* pData = *p_D2LAUNCH_BnData;
    if (pData) pData->nMaxDiff = 10;
  }
}

void __fastcall WhisperHandler(char* szAcc, char* szText) {
  if (!Vars.bDontCatchNextMsg) {
    wchar_t* szwText = AnsiToUnicode(szText, CP_ACP);
    WhisperEvent(szAcc, szwText);
    delete[] szwText;
  } else
    Vars.bDontCatchNextMsg = FALSE;
}

DWORD __fastcall GameAttack(UnitInteraction* pAttack) {
  if (!pAttack || !pAttack->lpTargetUnit || pAttack->lpTargetUnit->dwType != UNIT_MONSTER) return (DWORD)-1;

  if (pAttack->dwMoveType == ATTACKTYPE_UNITLEFT) pAttack->dwMoveType = ATTACKTYPE_SHIFTLEFT;

  if (pAttack->dwMoveType == ATTACKTYPE_RIGHT) pAttack->dwMoveType = ATTACKTYPE_SHIFTRIGHT;

  return NULL;
}

void __fastcall GamePlayerAssignment(UnitAny* pPlayer) {
  if (!pPlayer) return;

  PlayerAssignEvent(pPlayer->dwUnitId);
}

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD) {
  if (Vars.bGameLoopEntered) {
    LeaveCriticalSection(&Vars.cGameLoopSection);
  } else {
    Vars.bGameLoopEntered = true;
    Vars.dwGameThreadId = GetCurrentThreadId();
  }
  if (Vars.SectionCount) {
    Sleep(5);
  }

  EnterCriticalSection(&Vars.cGameLoopSection);
}

void GameLeave(void) {
  Vars.bQuitting = false;
  sScriptEngine->ForEachScript(StopIngameScript, NULL, 0);
  ActMap::ClearCache();
}

BOOL __fastcall RealmPacketRecv(BYTE* pPacket, DWORD dwSize) {
  return !RealmPacketEvent(pPacket, dwSize);
}

BOOL __fastcall ChatPacketRecv(BYTE* pPacket, [[maybe_unused]] int len) {
  bool blockPacket = false;

  if (pPacket[1] == 0xF) {
    // DWORD mode = pPacket[4];
    const char* who = (char*)pPacket + 28;
    char* said = (char*)pPacket + 29 + strlen(who);
    wchar_t* wsaid = AnsiToUnicode(said, CP_ACP);

    switch (pPacket[4]) {
      case 0x02:  // channel join
        ChatEvent(who, L"joined the channel");
        break;
      case 0x03:  // channel leave
        ChatEvent(who, L"left the channel");
        break;
      case 0x04:  // whispers
      case 0x0A:
        WhisperEvent(who, wsaid);
        break;
      case 0x05:  // normal text
      case 0x12:  // info blue text
      case 0x13:  // error message
      case 0x17:  // emoted text
        ChatEvent(who, wsaid);
        break;
      default:
        break;
    }
    delete[] wsaid;
    // ChannelEvent(mode,who,said);
  }

  return !blockPacket;
}
