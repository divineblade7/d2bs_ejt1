#include "d2bs/diablo/handlers/D2Handlers.h"

#include "d2bs/core/Control.h"
#include "d2bs/core/Core.h"
#include "d2bs/core/ScreenHook.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/handlers/D2NetHandlers.h"
#include "d2bs/new_util/localization.h"
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
    send = !((wMsg[0] == L'.' && ProcessCommand(wMsg + 1, false)) || FireChatInputEvent(wMsg));
  }

  return send ? 0 : -1;  // -1 means block, 0 means send
}

DWORD __fastcall ChannelInput(wchar_t* wMsg) {
  bool send = true;

  if (Vars.bDontCatchNextMsg)
    Vars.bDontCatchNextMsg = false;
  else {
    send = !((wMsg[0] == L'.' && ProcessCommand(wMsg + 1, false)) || FireChatInputEvent(wMsg));
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
      return !FireGamePacketEvent(pPacket, dwSize) && ReassignPlayerHandler(pPacket, dwSize);
    case 0x26:
      return !FireGamePacketEvent(pPacket, dwSize) && ChatEventHandler(pPacket, dwSize);
    case 0x2A:
      return !FireGamePacketEvent(pPacket, dwSize) && NPCTransactionHandler(pPacket, dwSize);
    case 0x5A:
      return !FireGamePacketEvent(pPacket, dwSize) && EventMessagesHandler(pPacket, dwSize);
    case 0x18:
    case 0x95:
      return !FireGamePacketEvent(pPacket, dwSize) && HPMPUpdateHandler(pPacket, dwSize);
    case 0x9C:
    case 0x9D:
      return !FireGamePacketEvent(pPacket, dwSize) && ItemActionHandler(pPacket, dwSize);
    case 0xA7:
      return !FireGamePacketEvent(pPacket, dwSize) && DelayedStateHandler(pPacket, dwSize);
  }

  return !FireGamePacketEvent(pPacket, dwSize);
}

DWORD __fastcall GamePacketSent(BYTE* pPacket, DWORD dwSize) {
  return !FireGamePacketSentEvent(pPacket, dwSize);
}

// TODO: Move into Console
void FlushPrint() {
  std::wstring str;
  while (sConsole->queue().dequeue_for(str, std::chrono::milliseconds(5))) {
    // Break into lines through \n.
    std::list<std::wstring> lines;
    std::wstring temp;
    std::wstringstream ss(str);

    if (Vars.settings.bUseGamePrint && ClientState() == ClientStateInGame) {
      while (getline(ss, temp)) {
        SplitLines(temp.c_str(), sConsole->MaxWidth() - 100, L' ', lines);
        sConsole->AddLine(temp);
      }

      // Convert and send every line.
      for (std::list<std::wstring>::iterator it = lines.begin(); it != lines.end(); ++it) {
        D2CLIENT_PrintGameString((wchar_t*)it->c_str(), 0);
      }
    } else {
      while (getline(ss, temp)) {
        sConsole->AddLine(temp);
      }
    }
  }
}

void SetMaxDiff(void) {
  if (D2CLIENT_GetDifficulty() == 1 && *p_D2CLIENT_ExpCharFlag) {
    BnetData* pData = *p_D2LAUNCH_BnData;
    if (pData) pData->nMaxDiff = 10;
  }
}

void __fastcall WhisperHandler(char* szAcc, char* szText) {
  if (!Vars.bDontCatchNextMsg) {
    auto szwText = d2bs::util::ansi_to_wide(szText);
    FireWhisperEvent(szAcc, szwText.c_str());
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

  FirePlayerAssignEvent(pPlayer->dwUnitId);
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
  sScriptEngine->for_each([](Script* script) {
    if (script->type() == ScriptType::InGame) {
      script->stop(true);
    }
    return true;
  });
  ActMap::ClearCache();
}

BOOL __fastcall RealmPacketRecv(BYTE* pPacket, DWORD dwSize) {
  return !FireRealmPacketEvent(pPacket, dwSize);
}

BOOL __fastcall ChatPacketRecv(BYTE* pPacket, [[maybe_unused]] int len) {
  bool blockPacket = false;

  if (pPacket[1] == 0xF) {
    // DWORD mode = pPacket[4];
    const char* who = (char*)pPacket + 28;
    char* said = (char*)pPacket + 29 + strlen(who);
    auto wsaid = d2bs::util::ansi_to_wide(said);

    switch (pPacket[4]) {
      case 0x02:  // channel join
        FireChatEvent(who, L"joined the channel");
        break;
      case 0x03:  // channel leave
        FireChatEvent(who, L"left the channel");
        break;
      case 0x04:  // whispers
      case 0x0A:
        FireWhisperEvent(who, wsaid.c_str());
        break;
      case 0x05:  // normal text
      case 0x12:  // info blue text
      case 0x13:  // error message
      case 0x17:  // emoted text
        FireChatEvent(who, wsaid.c_str());
        break;
      default:
        break;
    }
    // ChannelEvent(mode,who,said);
  }

  return !blockPacket;
}
