#pragma once

#include "d2bs/diablo/D2Ptrs.h"

UnitAny* GetUnit(char* szName, DWORD dwClassId, DWORD dwType, DWORD dwMode, DWORD dwUnitId);
UnitAny* GetNextUnit(UnitAny* pUnit, char* szName, DWORD dwClassId, DWORD dwType, DWORD dwMode);
UnitAny* GetInvUnit(UnitAny* pOwner, char* szName, DWORD dwClassId, DWORD dwMode, DWORD dwUnitId);
UnitAny* GetInvNextUnit(UnitAny* pUnit, UnitAny* pOwner, char* szName, DWORD dwClassId, DWORD dwMode);
BOOL CheckUnit(UnitAny* pUnit, char* szName, DWORD dwClassId, DWORD dwType, DWORD dwMode, DWORD dwUnitId);
UnitAny* D2CLIENT_FindUnit(DWORD dwId, DWORD dwType);

int GetUnitHP(UnitAny* pUnit);
int GetUnitMP(UnitAny* pUnit);
const char* GetUnitName(UnitAny* pUnit, char* szBuf, size_t bufSize);

UnitAny* GetPlayerUnit();
UnitAny* GetMercUnit(UnitAny* pUnit);

void GetItemCode(UnitAny* pUnit, char* szBuf);
int GetItemLocation(UnitAny* pItem);
