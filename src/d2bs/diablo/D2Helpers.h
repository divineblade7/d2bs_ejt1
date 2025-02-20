#pragma once

#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/D2Structs.h"
#include "d2bs/script/js32.h"

#define DEBUG_LOG(MESSAGE) Log(L"%s (%s, %d)", MESSAGE, __FILE__, __LINE__)

enum DistanceType { Euclidean, Chebyshev, Manhattan };

enum ClientGameState { ClientStateNull, ClientStateMenu, ClientStateInGame, ClientStateBusy };

void Log(const wchar_t* szFormat, ...);
void LogNoFormat(const wchar_t* szString);

// Game functions (used absolutely everywhere!)
ClientGameState ClientState(void);
bool GameReady(void);
bool WaitForGameReady(void);

// Area, Act, Level, Room
// return level id
DWORD GetPlayerArea(void);
Level* GetLevel(DWORD dwLevelNo);
DWORD GetTileLevelNo(Room2* lpRoom2, DWORD dwTileNo);
void AddRoomData(Room2* room);
void RemoveRoomData(Room2* room);
char* __stdcall GetLevelName(const Level* level);
char* __stdcall GetLevelIdName(DWORD level);

// Input/output
void SendMouseClick(int x, int y, int clicktype);
void SendKeyPress(uint type, uint key, uint ext);
BOOL ClickNPCMenu(DWORD NPCClassId, DWORD MenuId);

// Screen
void WorldToScreen(POINT* pPos);
void ScreenToWorld(POINT* ptPos);
POINT ScreenToAutomap(int x, int y);
void AutomapToScreen(POINT* pPos);
POINT CalculateTextLen(const char* szwText, int Font);
POINT CalculateTextLen(const wchar_t* szwText, int Font);
void myDrawText(const wchar_t* szwText, int x, int y, int color, int font);
void myDrawCenterText(const wchar_t* szwText, int x, int y, int color, int font, int div);
POINT GetScreenSize();
int D2GetScreenSizeX();
int D2GetScreenSizeY();
bool IsScrollingText();

// Automap
void myDrawAutomapCell(CellFile* cellfile, int xpos, int ypos, BYTE col);
AutomapLayer* InitAutomapLayer(DWORD levelno);
DWORD __fastcall D2CLIENT_InitAutomapLayer_STUB(DWORD nLayerNo);

// Skill
int GetSkill(WORD wSkillId);
BOOL SetSkill(JSContext* cx, WORD wSkillId, BOOL bLeft, DWORD dwItemId = 0xFFFFFFFF);
char* GetSkillByID(WORD id);
WORD GetSkillByName(char* szSkillName);

// utils
BYTE CalcPercent(DWORD dwVal, DWORD dwMaxVal, BYTE iMin = NULL);
double GetDistance(long x1, long y1, long x2, long y2, DistanceType type = Euclidean);
void ReadProcessBYTES(HANDLE hProcess, DWORD lpAddress, void* buf, int len);

// filesystem
CellFile* LoadCellFile(const char* lpszPath, DWORD bMPQ = TRUE);
CellFile* LoadCellFile(const wchar_t* lpszPath, DWORD bMPQ = 3);
DWORD ReadFile(HANDLE hFile, void* buf, DWORD len);
void* memcpy2(void* dest, const void* src, size_t count);
HANDLE OpenFileRead(const char* filename);
HANDLE OpenFileRead(const wchar_t* filename);
BYTE* AllocReadFile(const char* filename);
BYTE* AllocReadFile(const wchar_t* filename);
CellFile* LoadBmpCellFile(BYTE* buf1, int width, int height);
CellFile* LoadBmpCellFile(const char* filename);
CellFile* LoadBmpCellFile(const wchar_t* filename);
CellFile* myInitCellFile(CellFile* cf);

// Stubs
void __fastcall D2CLIENT_ClickBelt(DWORD x, DWORD y, Inventory* pInventoryData);
void __fastcall D2CLIENT_ClickBeltRight_ASM(DWORD pInventory, DWORD pPlayer, DWORD HoldShift, DWORD dwPotPos);
DWORD __cdecl D2CLIENT_GetMinionCount(UnitAny* pUnit, DWORD dwType);
void __fastcall D2CLIENT_HostilePartyUnit(RosterUnit* pUnit, DWORD dwButton);
void __stdcall D2CLIENT_TakeWaypoint(DWORD dwWaypointId, DWORD dwArea);
void __stdcall D2CLIENT_LeftClickItem(DWORD Location, UnitAny* pPlayer, Inventory* pInventory, int x, int y,
                                      DWORD dwClickType, InventoryLayout* pLayout);

// Possibly unused stubs
DWORD __fastcall D2CLIENT_GetUnitName_STUB(DWORD UnitAny);
DWORD __fastcall D2CLIENT_GetUIVar_STUB(DWORD varno);
void __fastcall D2CLIENT_SetSelectedUnit_STUB(DWORD UnitAny);
void __fastcall D2CLIENT_Interact_ASM(DWORD Struct);
DWORD __fastcall D2CLIENT_ClickParty_ASM(DWORD RosterUnit, DWORD Mode);
void __fastcall D2CLIENT_ClickShopItem_ASM(DWORD x, DWORD y, DWORD BuyOrSell);
void __fastcall D2CLIENT_ShopAction_ASM(DWORD pTable, DWORD pItem, DWORD pNPC, DWORD _1,
                                        DWORD pTable2 /* Could be also the ItemCost?*/, DWORD dwMode, DWORD _2,
                                        DWORD _3);
void __fastcall D2CLIENT_ClickItemRight_ASM(DWORD x, DWORD y, DWORD Location, DWORD pItem, DWORD pItemPath);

void __fastcall D2CLIENT_GetItemDesc_ASM(DWORD pUnit, wchar_t* pBuffer);
void __fastcall D2COMMON_DisplayOverheadMsg_ASM(DWORD pUnit);
void __fastcall D2CLIENT_MercItemAction_ASM(DWORD bPacketType, DWORD dwSlot);
void __fastcall D2CLIENT_PlaySound(DWORD dwSoundId);
void __fastcall D2GFX_DrawRectFrame_STUB(RECT* rect);
DWORD __fastcall D2CLIENT_SendGamePacket_ASM(DWORD dwLen, BYTE* bPacket);
