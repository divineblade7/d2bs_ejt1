#include "d2bs/engine.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/handlers/D2Handlers.h"
#include "d2bs/utils/Helpers.h"

#include <filesystem>

void __declspec(naked) RealmPacketRecv_Interception() {
  __asm {
		test esi, esi
		jz block
		pushad

		call RealmPacketRecv
		cmp eax, 0

		popad
		je Block
		call esi
Block:
		ret
  }
}

void __declspec(naked) GamePacketReceived_Intercept() {
  __asm {
		pushad;

		call GamePacketReceived;
		test eax, eax;

		popad;
		jnz OldCode;

		mov edx, 0;

	OldCode:
		mov eax, [esp + 4];
		push eax;
		call D2NET_ReceivePacket_I;

		ret 4;
  }
}

void __declspec(naked) GamePacketSent_Interception() {
  __asm {
		pushf
		pushad;

		mov ecx, [esp + 0x22 + 4 + 0xC];
		mov edx, [esp + 0x22 + 4 + 0x4];
		call GamePacketSent;
		test eax, eax;

		popad;
		jnz OldCode;

		mov[esp + 0x4 + 4 + 2], 0;

	OldCode:
		popf
			jnz good
			xor eax, eax
			pop ebp
			ret 0Ch
			good :
		jmp D2CLIENT_SendPacket_II
  }
}

void GameDraw_Intercept(void) {
  sEngine->on_game_draw();
}

void __declspec(naked) GameInput_Intercept() {
  __asm {
		pushad
		mov ecx, ebx
		call GameInput
		cmp eax, -1
		popad
		je BlockIt
		call D2CLIENT_InputCall_I
		ret

BlockIt:
		xor eax,eax
		ret
  }
}

UnitAny* GetSelectedUnit_Intercept(void) {
  if (Vars.bClickAction) {
    if (Vars.dwSelectedUnitId) {
      UnitAny* pUnit = D2CLIENT_FindUnit(Vars.dwSelectedUnitId, Vars.dwSelectedUnitType);

      return pUnit;
    }

    return NULL;
  }

  return D2CLIENT_GetSelectedUnit();
}

void __declspec(naked) Whisper_Intercept() {
  __asm
  {
		mov [esp-674h-4+4], edi
		pop edi
		sub esp, 678h
		pushad
		mov ecx, ebx
		mov edx, [ebp+8]
		call WhisperHandler
		popad
        // jmp D2MULTI_WhisperIntercept_Jump
		jmp edi
  }
}

VOID __declspec(naked) ChatPacketRecv_Interception() {
  __asm {
		mov edx, [ebp + 8]
		mov ecx, ebx
		pushad
		sub ecx, 4
		add edx, 4

		call ChatPacketRecv
		test eax, eax
		popad
 
		je Block
		call edi
Block:
		ret
  }
}

void __declspec(naked) GameAttack_Intercept() {
  __asm {
		pushad
		mov ecx, esi  // attack struct
		call GameAttack
		cmp eax, -1
		popad
		je OldCode

		call D2CLIENT_GetSelectedUnit
		
		cmp eax, 0
		je OldCode

		mov [esp+0x08+0x4+0x4], 1  // bool unit

OldCode:
		mov eax, [p_D2CLIENT_MouseY]
		mov eax, [eax]
		retn
  }
}

void __declspec(naked) PlayerAssignment_Intercept() {
  __asm
  {
		FNOP
		CALL D2CLIENT_AssignPlayer_I
		MOV ECX, EAX
		CALL GamePlayerAssignment
		RETN
  }
}

void __declspec(naked) GameCrashFix_Intercept() {
  __asm {
		CMP ECX, 0
		JE Skip
		MOV DWORD PTR DS:[ECX+0x10],EDX
Skip:
		MOV DWORD PTR DS:[EAX+0xC],0
		RETN
  }
}

void GameDrawOOG_Intercept(void) {
  sEngine->on_menu_draw();
}

void __declspec(naked) CongratsScreen_Intercept(void) {
  __asm
  {
		call D2CLIENT_CongratsScreen_I
		pushad
		call SetMaxDiff
		popad
		retn
  }
}

void __declspec(naked) GameActChange_Intercept(void) {
  __asm
  {
		POP EAX
		PUSH ESI
		XOR ESI, ESI
		CMP [Vars.bChangedAct], 0
		MOV [Vars.bChangedAct], 0
		JMP EAX
  }
}

void __declspec(naked) GameActChange2_Intercept(void) {
  __asm
  {
		MOV ESP, EBP
		POP EBP
		MOV [Vars.bChangedAct], 1
		retn
  }
}

void __declspec(naked) GameLeave_Intercept(void) {
  __asm
  {
		call GameLeave
		jmp D2CLIENT_GameLeave_I
  }
}

void __declspec(naked) ChannelInput_Intercept(void) {
  __asm {
		push ecx
		mov ecx, esi

		call ChannelInput

		test eax, eax
		pop ecx

		jz SkipInput
		call D2MULTI_ChannelInput_I

SkipInput:
		ret
  }
}

// flow in or out of inline asm code suppresses global optimization
#pragma warning(push)
#pragma warning(disable : 4740)
VOID __declspec(naked) __fastcall ClassicSTUB() {
  *p_BNCLIENT_ClassicKey = Vars.szClassic;
  __asm {
		
		jmp BNCLIENT_DClass;
  }
}

VOID __declspec(naked) __fastcall LodSTUB() {
  *p_BNCLIENT_XPacKey = Vars.szLod;
  __asm {
		
		jmp BNCLIENT_DLod;
  }
}
#pragma warning(pop)

void __declspec(naked) FailToJoin() {
  __asm
  {
		cmp esi, 4000;
		ret;
  }
}

int EraseCacheFiles() {
  CHAR cur_dir[MAX_PATH]{};
  GetCurrentDirectoryA(MAX_PATH, cur_dir);
  std::filesystem::path cur_path = cur_dir;
  if (!std::filesystem::exists(cur_path) || !std::filesystem::is_directory(cur_path)) {
    return 0;
  }

  for (const auto& entry : std::filesystem::directory_iterator(cur_path))
    if (std::filesystem::is_regular_file(entry) && entry.path().has_extension() &&
        entry.path().extension().compare(".dat") == 0) {
      std::filesystem::remove(entry);
    }

  return 0;
}

HMODULE __stdcall Multi(LPSTR, LPSTR) {
  return 0;
}

HANDLE __stdcall Windowname(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR, DWORD dwStyle, int x, int y, int nWidth,
                            int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
  WCHAR szWindowName[200] = L"D2";
  WCHAR szClassName[200] = L"CNAME";

  if (wcslen(Vars.szTitle) > 1) wcscpy_s(szWindowName, _countof(szWindowName), Vars.szTitle);

  WCHAR* wClassName = AnsiToUnicode(lpClassName);
  wcscpy_s(szClassName, _countof(szClassName), wClassName);
  delete[] wClassName;

  return CreateWindowExW(dwExStyle, szClassName, szWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu,
                         hInstance, lpParam);
}

HANDLE __stdcall CacheFix(LPCSTR, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                          DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
  EraseCacheFiles();
  CHAR path[MAX_PATH];
  GetCurrentDirectoryA(MAX_PATH, path);

  if (Vars.bCacheFix) {
    char szTitle[128];
    GetWindowText(D2GFX_GetHwnd(), szTitle, 128);
    CHAR Def[100] = "";

    if (strlen(szTitle) > 1) {
      sprintf_s(Def, "\\bncache%s.dat", szTitle);
      strcat_s(path, Def);
    } else {
      srand(GetTickCount());
      sprintf_s(Def, "\\bncache%d.dat", rand() % 0x2000);

      strcat_s(path, Def);
    }
  } else
    strcat_s(path, "\\bncache.dat");

  return CreateFileA(path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
                     dwFlagsAndAttributes, hTemplateFile);
}
