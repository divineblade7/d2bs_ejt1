#pragma once

#include "d2bs/diablo/D2Structs.h"

#include <windows.h>

DWORD __fastcall GameInput(wchar_t* wMsg);
DWORD __fastcall GamePacketReceived(BYTE* pPacket, DWORD dwSize);
DWORD __fastcall GamePacketSent(BYTE* pPacket, DWORD dwSize);
void FlushPrint();
  void SetMaxDiff(void);
void __fastcall WhisperHandler(char* szAcc, char* szText);
DWORD __fastcall ChannelInput(wchar_t* wMsg);
DWORD __fastcall GameAttack(UnitInteraction* pAttack);
void __fastcall GamePlayerAssignment(UnitAny* pPlayer);
void GameLeave(void);
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID __fastcall ClassicSTUB();
VOID __fastcall LodSTUB();
void FailToJoin();
HMODULE __stdcall Multi(LPSTR Class, LPSTR Window);
HANDLE __stdcall Windowname(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y,
                            int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HANDLE __stdcall CacheFix(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL __fastcall RealmPacketRecv(BYTE* pPacket, DWORD dwSize);
BOOL __fastcall ChatPacketRecv(BYTE* pPacket, int len);

char __fastcall ErrorReportLaunch(const char* crash_file, int a2);
