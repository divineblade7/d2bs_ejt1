#pragma once

#include "d2bs/diablo/D2Ptrs.h"

#include <windows.h>

void SendGold(int nGold, int nMode);
void __fastcall UseStatPoint(WORD stat, DWORD count = 1);
void __fastcall UseSkillPoint(WORD skill, DWORD count = 1);
