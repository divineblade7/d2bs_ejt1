#include "d2bs/diablo/D2Helpers.h"

#include "d2bs/engine.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Skills.h"
#include "d2bs/diablo/patches/D2Intercepts.h"
#include "d2bs/utils/CriticalSections.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/utils/stringhash.h"

#include <cmath>
#include <ctime>
#include <errno.h>
#include <io.h>
#include <sstream>
#include <string>

void Log(const wchar_t* szFormat, ...) {
  va_list vaArgs;

  va_start(vaArgs, szFormat);
  int len = _vscwprintf(szFormat, vaArgs);
  wchar_t* szString = new wchar_t[len + 1];
  vswprintf_s(szString, len + 1, szFormat, vaArgs);
  va_end(vaArgs);

  if (len > 0 && szString[len - 1] == L'\n') szString[len - 1] = 0;

  LogNoFormat(szString);
  delete[] szString;
}

void LogNoFormat(const wchar_t* szString) {
  time_t tTime;
  struct tm timestamp;
  char szTime[128] = "";
  wchar_t path[_MAX_PATH + _MAX_FNAME] = L"";
  time(&tTime);
  localtime_s(&timestamp, &tTime);
  strftime(szTime, sizeof(szTime), "%Y%m%d", &timestamp);
  swprintf_s(path, _countof(path), L"%s\\d2bs-%s-%S.log", Vars.log_dir.wstring().c_str(), Vars.szProfile, szTime);
#ifdef DEBUG
  FILE* log = stderr;
#else
  FILE* log = _wfsopen(path, L"a+", _SH_DENYNO);
#endif
  static DWORD id = GetProcessId(GetCurrentProcess());
  char* sString = UnicodeToAnsi(szString);
  strftime(szTime, sizeof(szTime), "%x %X", &timestamp);
  fprintf(log, "[%s] D2BS %d: %s\n", szTime, id, sString);
  delete[] sString;
#ifndef DEBUG
  fflush(log);
  fclose(log);
#endif
}

bool InArea(int x, int y, int x2, int y2, int sizex, int sizey) {
  return !!(x >= x2 && x < x2 + sizex && y >= y2 && y < y2 + sizey);
}

/*UnitAny* FindItemByPosition(DWORD x, DWORD y, DWORD Location) {
for(UnitAny* pItem = D2COMMON_GetItemFromInventory(D2CLIENT_GetPlayerUnit()->pInventory); pItem; pItem =
D2COMMON_GetNextItemFromInventory(pItem)) { if((DWORD)GetItemLocation(pItem) == Location &&
InArea(x,y,pItem->pObjectPath->dwPosX,pItem->pObjectPath->dwPosY,D2COMMON_GetItemText(pItem->dwTxtFileNo)->xSize,D2COMMON_GetItemText(pItem->dwTxtFileNo)->ySize))
return pItem;
}
return NULL;
}*/

/*void SelectInventoryItem(DWORD x, DWORD y, DWORD dwLocation)
{
*(DWORD*)&p_D2CLIENT_SelectedInvItem = (DWORD)FindItemByPosition(x, y, dwLocation);
}*/

// Do not edit without the express consent of bsdunx or lord2800
ClientGameState ClientState(void) {
  ClientGameState state = ClientStateNull;
  UnitAny* player = D2CLIENT_GetPlayerUnit();
  Control* firstControl = *p_D2WIN_FirstControl;

  if (player && !firstControl) {
    if (player && player->pUpdateUnit) {
      state = ClientStateBusy;
      return state;
    }
    if (player->pInventory && player->pPath &&
        // player->pPath->xPos &&
        player->pPath->pRoom1 && player->pPath->pRoom1->pRoom2 && player->pPath->pRoom1->pRoom2->pLevel &&
        player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo)
      state = ClientStateInGame;
    else
      state = ClientStateBusy;
  } else if (!player && firstControl)
    state = ClientStateMenu;
  else if (!player && !firstControl)
    state = ClientStateNull;

  return state;
}

bool GameReady(void) {
  return (ClientState() == ClientStateInGame ? true : false);
}

bool WaitForGameReady(void) {
  DWORD start = GetTickCount();
  do {
    switch (ClientState()) {
      case ClientStateNull:
      case ClientStateMenu:
        return false;
      case ClientStateInGame:
        return true;
    }
    Sleep(10);
  } while ((Vars.dwGameTimeout == 0) || (Vars.dwGameTimeout > 0 && (GetTickCount() - start) < Vars.dwGameTimeout));
  return false;
}

DWORD GetPlayerArea(void) {
  return (ClientState() == ClientStateInGame ? D2CLIENT_GetPlayerUnit()->pPath->pRoom1->pRoom2->pLevel->dwLevelNo
                                             : NULL);
}

Level* GetLevel(DWORD dwLevelNo) {
  CriticalRoom cRoom;

  if (!GameReady()) return nullptr;

  Level* pLevel = D2CLIENT_GetPlayerUnit()->pAct->pMisc->pLevelFirst;

  while (pLevel) {
    if (pLevel->dwLevelNo == dwLevelNo) {
      if (!pLevel->pRoom2First) D2COMMON_InitLevel(pLevel);

      if (!pLevel->pRoom2First) break;
      return pLevel;
    }
    pLevel = pLevel->pNextLevel;
  }

  // this crashes pretty much every time it's called
  // pLevel = D2COMMON_GetLevel(D2CLIENT_GetPlayerUnit()->pAct->pMisc, dwLevelNo);
  return pLevel;
}

// TODO: make this use SIZE for clarity
POINT CalculateTextLen(const char* szwText, int Font) {
  POINT ret = {0, 0};

  if (!szwText) return ret;

  wchar_t* buf = AnsiToUnicode(szwText);
  ret = CalculateTextLen(buf, Font);
  delete[] buf;
  return ret;
}

POINT CalculateTextLen(const wchar_t* szwText, int Font) {
  POINT ret = {0, 0};

  if (!szwText) return ret;

  DWORD dwWidth, dwFileNo;
  DWORD dwOldSize = D2WIN_SetTextSize(Font);
  ret.y = D2WIN_GetTextSize((wchar_t*)szwText, &dwWidth, &dwFileNo);
  ret.x = dwWidth;
  D2WIN_SetTextSize(dwOldSize);

  return ret;
}

int GetSkill(WORD wSkillId) {
  if (!D2CLIENT_GetPlayerUnit()) return 0;

  for (Skill* pSkill = D2CLIENT_GetPlayerUnit()->pInfo->pFirstSkill; pSkill; pSkill = pSkill->pNextSkill)
    if (pSkill->pSkillInfo->wSkillId == wSkillId) return D2COMMON_GetSkillLevel(D2CLIENT_GetPlayerUnit(), pSkill, TRUE);

  return 0;
}

BOOL SetSkill(JSContext* cx, WORD wSkillId, BOOL bLeft, DWORD dwItemId) {
  if (ClientState() != ClientStateInGame) return FALSE;

  if (!GetSkill(wSkillId)) return FALSE;

  BYTE aPacket[9];

  aPacket[0] = 0x3C;
  *(WORD*)&aPacket[1] = wSkillId;
  aPacket[3] = 0;
  aPacket[4] = (bLeft) ? 0x80 : 0;
  *(DWORD*)&aPacket[5] = dwItemId;

  D2CLIENT_SendGamePacket(9, aPacket);

  UnitAny* Me = D2CLIENT_GetPlayerUnit();

  int timeout = 0;
  Skill* hand = NULL;
  while (ClientState() == ClientStateInGame) {
    hand = (bLeft ? Me->pInfo->pLeftSkill : Me->pInfo->pRightSkill);
    if (hand->pSkillInfo->wSkillId != wSkillId) {
      if (timeout > 10) return FALSE;
      timeout++;
    } else
      return TRUE;

    Script* script = (Script*)JS_GetContextPrivate(cx);  // run events to avoid packet block deadlock
    DWORD start = GetTickCount();
    int amt = 100 - (GetTickCount() - start);

    while (amt > 0) {  // had a script deadlock here, make sure were positve with amt
      WaitForSingleObjectEx(script->event_signal(), amt, true);
      ResetEvent(script->event_signal());

      // TEMPORARY: Still to much to detangle from the current event system to figure out where to put this call
      script->process_events();

      auto& events = script->events();
      while (events.size() > 0 && !!!(JSBool)(script->is_stopped() || ((script->type() == ScriptType::InGame) &&
                                                                      ClientState() == ClientStateMenu))) {
        EnterCriticalSection(&Vars.cEventSection);
        std::shared_ptr<Event> evt = events.back();
        events.pop_back();
        LeaveCriticalSection(&Vars.cEventSection);
        ExecScriptEvent(evt);
      }

      amt = 100 - (GetTickCount() - start);
      // SleepEx(10,true);	// ex for delayed setTimer
    }
  }

  return FALSE;
}

// Compare the skillname to the Game_Skills struct to find the right skill ID to return
WORD GetSkillByName(char* skillname) {
  for (int i = 0; i < 216; i++)
    if (_stricmp(Game_Skills[i].name, skillname) == 0) return Game_Skills[i].skillID;
  return (WORD)-1;
}

char* GetSkillByID(WORD id) {
  for (int i = 0; i < 216; i++)
    if (id == Game_Skills[i].skillID) return Game_Skills[i].name;
  return NULL;
}

void SendMouseClick(int x, int y, int clicktype) {
  // HACK: Using PostMessage instead of SendMessage--need to fix this ASAP!
  LPARAM lp = x + (y << 16);
  switch (clicktype) {
    case 0:
      PostMessage(D2GFX_GetHwnd(), WM_LBUTTONDOWN, 0, lp);
      break;
    case 1:
      PostMessage(D2GFX_GetHwnd(), WM_LBUTTONUP, 0, lp);
      break;
    case 2:
      PostMessage(D2GFX_GetHwnd(), WM_RBUTTONDOWN, 0, lp);
      break;
    case 3:
      PostMessage(D2GFX_GetHwnd(), WM_RBUTTONUP, 0, lp);
      break;
  }
}

void SendKeyPress(uint type, uint key, uint ext) {
  LPARAM lp = 1;
  lp |= ext << 24;
  lp |= (MapVirtualKey(key, MAPVK_VK_TO_VSC) << 16);

  if (type == WM_KEYUP) {
    lp |= 0xC0000000;
  }

  PostMessage(D2GFX_GetHwnd(), type, key, lp);
}

DWORD __declspec(naked) __fastcall D2CLIENT_InitAutomapLayer_STUB([[maybe_unused]] DWORD nLayerNo) {
  __asm
  {
		push edi;  // Updated 1.14d Registers changed.
		mov edi, ecx;
		call D2CLIENT_InitAutomapLayer_I;
		pop edi;
		ret;
  }
}

AutomapLayer* InitAutomapLayer(DWORD levelno) {
  AutomapLayer2* pLayer = D2COMMON_GetLayer(levelno);
  return D2CLIENT_InitAutomapLayer(pLayer->nLayerNo);
}

void WorldToScreen(POINT* pPos) {
  D2COMMON_MapToAbsScreen(&pPos->x, &pPos->y);
  pPos->x -= D2CLIENT_GetMouseXOffset();
  pPos->y -= D2CLIENT_GetMouseYOffset();
}

void ScreenToWorld(POINT* pPos) {
  D2COMMON_AbsScreenToMap(&pPos->x, &pPos->y);
  pPos->x += D2CLIENT_GetMouseXOffset();
  pPos->y += D2CLIENT_GetMouseYOffset();
}

POINT ScreenToAutomap(int x, int y) {
  POINT result = {0, 0};
  x *= 32;
  y *= 32;
  result.x = ((x - y) / 2 / (*p_D2CLIENT_Divisor)) - (*p_D2CLIENT_Offset).x + 8;
  result.y = ((x + y) / 4 / (*p_D2CLIENT_Divisor)) - (*p_D2CLIENT_Offset).y - 8;

  if (D2CLIENT_GetAutomapSize()) {
    --result.x;
    result.y += 5;
  }
  return result;
}

void AutomapToScreen(POINT* pPos) {
  pPos->x = 8 - p_D2CLIENT_Offset->x + (pPos->x * (*p_D2CLIENT_AutomapMode));
  pPos->y = 8 + p_D2CLIENT_Offset->y + (pPos->y * (*p_D2CLIENT_AutomapMode));
}

void myDrawText(const wchar_t* szwText, int x, int y, int color, int font) {
  DWORD dwOld = D2WIN_SetTextSize(font);
  D2WIN_DrawText(szwText, x, y, color, 0);
  D2WIN_SetTextSize(dwOld);
}

void myDrawCenterText(const wchar_t* szText, int x, int y, int color, int font, int div) {
  DWORD dwWidth = NULL, dwFileNo = NULL, dwOldSize = NULL;

  dwOldSize = D2WIN_SetTextSize(font);
  D2WIN_GetTextSize((wchar_t*)szText, &dwWidth, &dwFileNo);
  D2WIN_SetTextSize(dwOldSize);
  myDrawText(szText, x - (dwWidth >> div), y, color, font);
}

typedef void (*fnClickEntry)(void);

BOOL ClickNPCMenu(DWORD NPCClassId, DWORD MenuId) {
  NPCMenu* pMenu = (NPCMenu*)p_D2CLIENT_NPCMenu;
  fnClickEntry pClick = (fnClickEntry)NULL;

  for (UINT i = 0; i < *p_D2CLIENT_NPCMenuAmount; i++) {
    if (pMenu->dwNPCClassId == NPCClassId) {
      if (pMenu->wEntryId1 == MenuId) {
        pClick = (fnClickEntry)pMenu->dwEntryFunc1;
        if (pClick)
          pClick();
        else
          return FALSE;
        return TRUE;
      } else if (pMenu->wEntryId2 == MenuId) {
        pClick = (fnClickEntry)pMenu->dwEntryFunc2;
        if (pClick)
          pClick();
        else
          return FALSE;
        return TRUE;
      } else if (pMenu->wEntryId3 == MenuId) {
        pClick = (fnClickEntry)pMenu->dwEntryFunc3;
        if (pClick)
          pClick();
        else
          return FALSE;
        return TRUE;
      } else if (pMenu->wEntryId4 == MenuId) {
        pClick = (fnClickEntry)pMenu->dwEntryFunc4;
        if (pClick)
          pClick();
        else
          return FALSE;
        return TRUE;
      }
    }
    pMenu = (NPCMenu*)((DWORD)pMenu + sizeof(NPCMenu));
  }

  return FALSE;
}

BYTE CalcPercent(DWORD dwVal, DWORD dwMaxVal, BYTE iMin) {
  if (dwVal == 0 || dwMaxVal == 0) return 0;

  BYTE iRes = (BYTE)((double)dwVal / (double)dwMaxVal * 100.0);
  if (iRes > 100) iRes = 100;

  return std::max(iRes, iMin);
}

DWORD GetTileLevelNo(Room2* lpRoom2, DWORD dwTileNo) {
  for (RoomTile* pRoomTile = lpRoom2->pRoomTiles; pRoomTile; pRoomTile = pRoomTile->pNext) {
    if (*(pRoomTile->nNum) == dwTileNo) return pRoomTile->pRoom2->pLevel->dwLevelNo;
  }

  return NULL;
}

void AddRoomData(Room2* room) {
  D2COMMON_AddRoomData(room->pLevel->pMisc->pAct, room->pLevel->dwLevelNo, room->dwPosX, room->dwPosY, room->pRoom1);
}

void RemoveRoomData(Room2* room) {
  D2COMMON_RemoveRoomData(room->pLevel->pMisc->pAct, room->pLevel->dwLevelNo, room->dwPosX, room->dwPosY, room->pRoom1);
}

char* __stdcall GetLevelName(const Level* level) {
  return D2COMMON_GetLevelText(level->dwLevelNo)->szName;
}

char* __stdcall GetLevelIdName(DWORD level) {
  return D2COMMON_GetLevelText(level)->szName;
}

// TODO: Rewrite this and split it into two functions
CellFile* LoadCellFile(const char* lpszPath, DWORD bMPQ) {
  if (bMPQ == TRUE) {
    unsigned __int32 hash = sfh((char*)lpszPath, (int)strlen((char*)lpszPath));
    if (Vars.mCachedCellFiles.count(hash) > 0) {
      return Vars.mCachedCellFiles[hash];
    }
    CellFile* result = (CellFile*)D2WIN_LoadCellFile((char*)lpszPath, 0);
    Vars.mCachedCellFiles[hash] = result;
    return result;
  } else {
    wchar_t* path = AnsiToUnicode(lpszPath);
    CellFile* ret = LoadCellFile(path, bMPQ);
    delete[] path;
    return ret;
  }
}

CellFile* LoadCellFile(const wchar_t* lpszPath, DWORD bMPQ) {
  if (bMPQ == TRUE) {
    Log(L"Cannot specify wide character path for MPQ: %s", lpszPath);
    return NULL;
  }

  // AutoDetect the Cell File
  if (bMPQ == 3) {
    // Check in our directory first
    auto path = (Vars.script_dir / lpszPath).wstring();
    if (std::filesystem::exists(path)) {
      return LoadCellFile(path.c_str(), FALSE);
    } else {
      return LoadCellFile(lpszPath, TRUE);
    }
  }

  unsigned __int32 hash = sfh((char*)lpszPath, (int)strlen((char*)lpszPath));
  if (Vars.mCachedCellFiles.count(hash) > 0) {
    return Vars.mCachedCellFiles[hash];
  }

  // see if the file exists first
  if (!(_waccess(lpszPath, 0) != 0 && errno == ENOENT)) {
    CellFile* result = myInitCellFile((CellFile*)LoadBmpCellFile(lpszPath));
    Vars.mCachedCellFiles[hash] = result;
    return result;
  }

  return NULL;
}

POINT GetScreenSize() {
  // HACK: p_D2CLIENT_ScreenSize is wrong for out of game, which is hardcoded to 800x600
  POINT ingame = {static_cast<LONG>(*p_D2CLIENT_ScreenSizeX), static_cast<LONG>(*p_D2CLIENT_ScreenSizeY)},
        oog = {800, 600}, p = {0};
  if (ClientState() == ClientStateMenu)
    p = oog;
  else
    p = ingame;
  return p;
}

int D2GetScreenSizeX() {
  return GetScreenSize().x;
}

int D2GetScreenSizeY() {
  return GetScreenSize().y;
}

void myDrawAutomapCell(CellFile* cellfile, int xpos, int ypos, BYTE col) {
  if (!cellfile) return;
  CellContext ct;
  memset(&ct, 0, sizeof(ct));
  ct.pCellFile = cellfile;

  xpos -= (cellfile->cells[0]->width / 2);
  ypos += (cellfile->cells[0]->height / 2);

  int xpos2 = xpos - cellfile->cells[0]->xoffs, ypos2 = ypos - cellfile->cells[0]->yoffs;
  if ((xpos2 >= D2GetScreenSizeX()) || ((xpos2 + (int)cellfile->cells[0]->width) <= 0) ||
      (ypos2 >= D2GetScreenSizeY()) || ((ypos2 + (int)cellfile->cells[0]->height) <= 0))
    return;

  static BYTE coltab[2][256];  //, tabno = 0, lastcol = 0;
  if (!coltab[0][1])
    for (int k = 0; k < 255; k++) coltab[0][k] = coltab[1][k] = (BYTE)k;
  cellfile->mylastcol = coltab[cellfile->mytabno ^= (col != cellfile->mylastcol)][255] = col;

  D2GFX_DrawAutomapCell2(&ct, xpos, ypos, (DWORD)-1, 5, coltab[cellfile->mytabno]);
}

DWORD ReadFile(HANDLE hFile, void* buf, DWORD len)
// NOTE :- validates len bytes of buf
{
  DWORD numdone = 0;
  return ::ReadFile(hFile, buf, len, &numdone, NULL) != 0 ? numdone : -1;
}

void* memcpy2(void* dest, const void* src, size_t count) {
  return (char*)memcpy(dest, src, count) + count;
}

HANDLE OpenFileRead(const char* filename) {
  return CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

HANDLE OpenFileRead(const wchar_t* filename) {
  return CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

BYTE* AllocReadFile(const char* filename) {
  HANDLE hFile = OpenFileRead(filename);
  int filesize = GetFileSize(hFile, 0);
  if (filesize <= 0) return 0;
  BYTE* buf = new BYTE[filesize];
  ReadFile(hFile, buf, filesize);
  CloseHandle(hFile);
  return buf;
}

BYTE* AllocReadFile(const wchar_t* filename) {
  HANDLE hFile = OpenFileRead(filename);
  int filesize = GetFileSize(hFile, 0);
  if (filesize <= 0) return 0;
  BYTE* buf = new BYTE[filesize];
  ReadFile(hFile, buf, filesize);
  CloseHandle(hFile);
  return buf;
}

CellFile* LoadBmpCellFile(BYTE* buf1, int width, int height) {
  BYTE *buf2 = new BYTE[(width * height * 2) + height], *dest = buf2;

  for (int i = 0; i < height; i++) {
    BYTE *src = buf1 + (i * ((width + 3) & -4)), *limit = src + width;
    while (src < limit) {
      BYTE *start = src, *limit2 = std::min(limit, src + 0x7f), trans = !*src;
      do src++;
      while ((trans == (BYTE) !*src) && (src < limit2));
      if (!trans || (src < limit)) *dest++ = (BYTE)((trans ? 0x80 : 0) + (src - start));
      if (!trans)
        while (start < src) *dest++ = *start++;
    }
    *dest++ = 0x80;
  }

  static DWORD dc6head[] = {6, 1, 0, 0xeeeeeeee, 1, 1, 0x1c, 0, (DWORD)-1, (DWORD)-1, 0, 0, 0, (DWORD)-1, (DWORD)-1};
  dc6head[8] = width;
  dc6head[9] = height;
  dc6head[14] = dest - buf2;
  dc6head[13] = sizeof(dc6head) + (dc6head[14]) + 3;
  BYTE* ret = new BYTE[dc6head[13]];
  memset(memcpy2(memcpy2(ret, dc6head, sizeof(dc6head)), buf2, dc6head[14]), 0xee, 3);
  delete[] buf2;

  return (CellFile*)ret;
}

CellFile* LoadBmpCellFile(const char* filename) {
  BYTE* ret = 0;

  BYTE* buf1 = AllocReadFile(filename);
  BITMAPFILEHEADER* bmphead1 = (BITMAPFILEHEADER*)buf1;
  BITMAPINFOHEADER* bmphead2 = (BITMAPINFOHEADER*)(buf1 + sizeof(BITMAPFILEHEADER));
  if (buf1 && (bmphead1->bfType == 'MB') && (bmphead2->biBitCount == 8) && (bmphead2->biCompression == BI_RGB)) {
    ret = (BYTE*)LoadBmpCellFile(buf1 + bmphead1->bfOffBits, bmphead2->biWidth, bmphead2->biHeight);
  }
  delete[] buf1;

  return (CellFile*)ret;
}

CellFile* LoadBmpCellFile(const wchar_t* filename) {
  BYTE* ret = 0;

  BYTE* buf1 = AllocReadFile(filename);
  BITMAPFILEHEADER* bmphead1 = (BITMAPFILEHEADER*)buf1;
  BITMAPINFOHEADER* bmphead2 = (BITMAPINFOHEADER*)(buf1 + sizeof(BITMAPFILEHEADER));
  if (buf1 && (bmphead1->bfType == 'MB') && (bmphead2->biBitCount == 8) && (bmphead2->biCompression == BI_RGB)) {
    ret = (BYTE*)LoadBmpCellFile(buf1 + bmphead1->bfOffBits, bmphead2->biWidth, bmphead2->biHeight);
  }
  delete[] buf1;

  return (CellFile*)ret;
}

CellFile* myInitCellFile(CellFile* cf) {
  if (cf) D2CMP_InitCellFile(cf, &cf, "?", 0, (DWORD)-1, "?");
  return cf;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// End of Sting's or Mousepad's
///////////////////////////////////////////////////////////////////////////////////////////////////

DWORD __declspec(naked) __fastcall D2CLIENT_GetUnitName_STUB([[maybe_unused]] DWORD UnitAny) {
  __asm
  {
    // mov eax, ecx
			jmp D2CLIENT_GetUnitName_I
  }
}

DWORD __declspec(naked) __fastcall D2CLIENT_GetUIVar_STUB([[maybe_unused]] DWORD varno) {
  __asm
  {
		mov eax, ecx;
		jmp D2CLIENT_GetUiVar_I;
  }
}

void __declspec(naked) __fastcall D2CLIENT_SetSelectedUnit_STUB([[maybe_unused]] DWORD UnitAny) {
  __asm
  {
		mov eax, ecx
			jmp D2CLIENT_SetSelectedUnit_I
  }
}
// DWORD __declspec(naked) __fastcall D2CLIENT_LoadUIImage_ASM(char* Path)
//{
//	__asm {
//		mov eax, ecx
//			push 0
//			call D2CLIENT_LoadUIImage_I
//			retn
//	}
//}

void __declspec(naked) __fastcall D2CLIENT_Interact_ASM([[maybe_unused]] DWORD Struct) {
  __asm {
		mov esi, ecx
			jmp D2CLIENT_Interact_I
  }
}

DWORD __declspec(naked) __fastcall D2CLIENT_ClickParty_ASM([[maybe_unused]] DWORD RosterUnit,
                                                           [[maybe_unused]] DWORD Mode) {
  __asm
  {
		mov eax, ecx
			jmp D2CLIENT_ClickParty_I
  }
}

// obsoleted - use D2CLIENT_ShopAction instead
// This isn't finished anyway!
void __declspec(naked) __fastcall D2CLIENT_ClickShopItem_ASM([[maybe_unused]] DWORD x, [[maybe_unused]] DWORD y,
                                                             [[maybe_unused]] DWORD BuyOrSell) {
  __asm
  {
		mov esi, ecx
			mov edi, edx
			pop eax  // Save return address to eax
			pop ecx  // Buy or sell to ecx
			push ecx
			push 1
			push eax
			jmp D2CLIENT_ClickShopItem_I
  }
}

void __declspec(naked) __fastcall D2CLIENT_ShopAction_ASM(
    [[maybe_unused]] DWORD pItem, [[maybe_unused]] DWORD pNpc, [[maybe_unused]] DWORD pNPC, [[maybe_unused]] DWORD _1,
    [[maybe_unused]] DWORD pTable2 /* Could be also the ItemCost?*/, [[maybe_unused]] DWORD dwMode,
    [[maybe_unused]] DWORD _2, [[maybe_unused]] DWORD _3) {
  __asm {
		jmp D2CLIENT_ShopAction_I
  }
}

void __declspec(naked) __fastcall D2CLIENT_ClickBelt([[maybe_unused]] DWORD x, [[maybe_unused]] DWORD y,
                                                     [[maybe_unused]] Inventory* pInventoryData) {
  __asm {
		mov eax, edx
			jmp D2CLIENT_ClickBelt_I
  }
}

void __declspec(naked) __stdcall D2CLIENT_LeftClickItem([[maybe_unused]] DWORD Location,
                                                        [[maybe_unused]] UnitAny* pPlayer,
                                                        [[maybe_unused]] Inventory* pInventory, [[maybe_unused]] int x,
                                                        [[maybe_unused]] int y, [[maybe_unused]] DWORD dwClickType,
                                                        [[maybe_unused]] InventoryLayout* pLayout) {
  __asm
  {
		pop eax  // pop return address
		xchg eax, [esp]  // return address to stack, location to eax
		jmp D2CLIENT_LeftClickItem_I
  }
}

void __declspec(naked) __fastcall D2CLIENT_ClickItemRight_ASM([[maybe_unused]] DWORD x, [[maybe_unused]] DWORD y,
                                                              [[maybe_unused]] DWORD Location,
                                                              [[maybe_unused]] DWORD Player,
                                                              [[maybe_unused]] DWORD pUnitInventory) {
  __asm
  {
		xchg edx, ecx  // x, y -> y, x
		pop eax  // pop return address
		xchg eax, [esp]  // return address to stack, location to eax
		jmp D2CLIENT_ClickItemRight_I
  }
}

void __declspec(naked) __fastcall D2CLIENT_ClickBeltRight_ASM([[maybe_unused]] DWORD pInventory,
                                                              [[maybe_unused]] DWORD pPlayer,
                                                              [[maybe_unused]] DWORD HoldShift,
                                                              [[maybe_unused]] DWORD dwPotPos) {
  __asm
  {
		pop eax  // pop return address
		xchg eax, [esp]  // return address to stack, location to eax
		jmp D2CLIENT_ClickBeltRight_I
  }
}

void __declspec(naked) __fastcall D2CLIENT_GetItemDesc_ASM([[maybe_unused]] DWORD pUnit,
                                                           [[maybe_unused]] wchar_t* pBuffer) {
  __asm
  {
		PUSH EDI
			MOV EDI, EDX
			PUSH NULL
			PUSH 1  // TRUE = New lines, FALSE = Comma between each stat value
			PUSH ECX
			CALL D2CLIENT_GetItemDesc_I
			POP EDI
			RETN
  }
}

void __declspec(naked) __fastcall D2COMMON_DisplayOverheadMsg_ASM([[maybe_unused]] DWORD pUnit) {
  __asm
  {
		LEA ESI, [ECX+0xA4]
		MOV EAX, [ECX+0xA4]

		PUSH EAX
			PUSH 0
			call D2COMMON_DisplayOverheadMsg_I

			RETN
  }
}

void __declspec(naked) __fastcall D2CLIENT_MercItemAction_ASM([[maybe_unused]] DWORD bPacketType,
                                                              [[maybe_unused]] DWORD dwSlot) {
  __asm
  {
    // mov eax, ecx
    // mov ecx, edx
			jmp D2CLIENT_MercItemAction_I
  }
}

void __declspec(naked) __fastcall D2CLIENT_PlaySound([[maybe_unused]] DWORD dwSoundId) {
  __asm
  {
		MOV EBX, ECX
			PUSH NULL
			PUSH NULL
			PUSH NULL
			MOV EAX, p_D2CLIENT_PlayerUnit
			MOV EAX, [EAX]
		MOV ECX, EAX
			MOV EDX, EBX
        // CALL D2CLIENT_PlaySound_I
			RETN
  }
}

__declspec(naked) void __stdcall D2CLIENT_TakeWaypoint([[maybe_unused]] DWORD dwWaypointId,
                                                       [[maybe_unused]] DWORD dwArea) {
  __asm {
		PUSH EBP
			MOV EBP, ESP
			SUB ESP, 0x20
			PUSH EBX
			PUSH ESI
			PUSH EDI
			AND DWORD PTR SS:[EBP-0x20],0
			PUSH 0
			CALL _TakeWaypoint
			POP EDI
			POP ESI
			POP EBX
			LEAVE
			RETN 8

_TakeWaypoint:
		PUSH EBP
			PUSH EBX
			PUSH ESI
			PUSH EDI

			XOR EDI, EDI
			MOV EBX, 1
			MOV EDX, DWORD PTR SS : [EBP + 8]  // waypointId
			MOV ECX, DWORD PTR SS : [EBP + 0xC]  // area
			PUSH ECX
			LEA ESI, DWORD PTR SS : [EBP - 0x20]  // struct
		JMP [D2CLIENT_TakeWaypoint_I]

  }
}
/*DWORD __declspec(naked) __fastcall D2CLIENT_TestPvpFlag_STUB(DWORD planum1, DWORD planum2, DWORD flagmask)
{
        __asm
        {
                push esi;
                push [esp+8];
                mov esi, edx; // p2
                mov edx, ecx; // p1
                call D2CLIENT_TestPvpFlag_I;
                pop esi;
                ret 4;
        }
}*/

void __declspec(naked) __fastcall D2GFX_DrawRectFrame_STUB([[maybe_unused]] RECT* rect) {
  __asm
  {
		mov eax, ecx;
		jmp D2CLIENT_DrawRectFrame;
  }
}

DWORD __cdecl D2CLIENT_GetMinionCount([[maybe_unused]] UnitAny* pUnit, [[maybe_unused]] DWORD dwType) {
  DWORD dwResult;

  __asm
  {
		push eax
			push ecx
			push edx
			mov ecx, pUnit
			mov edx, dwType
			call D2CLIENT_GetMinionCount_I
			mov [dwResult], eax
			pop edx
			pop ecx
			pop eax
  }

  return dwResult;
}

__declspec(naked) void __fastcall D2CLIENT_HostilePartyUnit([[maybe_unused]] RosterUnit* pUnit,
                                                            [[maybe_unused]] DWORD dwButton) {
  __asm
  {
		mov eax, edx
			jmp [D2CLIENT_ClickParty_II]
  }
}

__declspec(naked) DWORD
    __fastcall D2CLIENT_SendGamePacket_ASM([[maybe_unused]] DWORD dwLen, [[maybe_unused]] BYTE* bPacket) {
  __asm
  {
		push edi
			mov edi, ecx
			push edx
			call D2CLIENT_SendGamePacket_I
			pop edi
			ret
  }
}

double GetDistance(long x1, long y1, long x2, long y2, DistanceType type) {
  double dist = 0;
  switch (type) {
    case Euclidean: {
      double dx = (double)(x2 - x1);
      double dy = (double)(y2 - y1);
      dx = pow(dx, 2);
      dy = pow(dy, 2);
      dist = sqrt(dx + dy);
    } break;
    case Chebyshev: {
      long dx = (x2 - x1);
      long dy = (y2 - y1);
      dx = abs(dx);
      dy = abs(dy);
      dist = std::max(dx, dy);
    } break;
    case Manhattan: {
      long dx = (x2 - x1);
      long dy = (y2 - y1);
      dx = abs(dx);
      dy = abs(dy);
      dist = (dx + dy);
    } break;
    default:
      dist = -1;
      break;
  }
  return dist;
}

bool IsScrollingText() {
  if (!WaitForGameReady()) return false;

  HWND d2Hwnd = D2GFX_GetHwnd();
  WindowHandlerList* whl = p_STORM_WindowHandlers->table[(0x534D5347 ^ (DWORD)d2Hwnd) % p_STORM_WindowHandlers->length];
  MessageHandlerHashTable* mhht;
  MessageHandlerList* mhl;

  while (whl) {
    if (whl->unk_0 == 0x534D5347 && whl->hWnd == d2Hwnd) {
      mhht = whl->msgHandlers;
      if (mhht != NULL && mhht->table != NULL && mhht->length != 0) {
        // 0x201 - WM_something click
        mhl = mhht->table[0x201 % mhht->length];

        if (mhl != NULL) {
          while (mhl) {
            if (mhl->message && mhl->unk_4 < 0xffffffff && mhl->handler == D2CLIENT_CloseNPCTalk) {
              return true;
            }
            mhl = mhl->next;
          }
        }
      }
    }
    whl = whl->next;
  }

  return false;
}

void ReadProcessBYTES(HANDLE hProcess, DWORD lpAddress, void* buf, int len) {
  DWORD oldprot, dummy = 0;
  VirtualProtectEx(hProcess, (void*)lpAddress, len, PAGE_READWRITE, &oldprot);
  ReadProcessMemory(hProcess, (void*)lpAddress, buf, len, 0);
  VirtualProtectEx(hProcess, (void*)lpAddress, len, oldprot, &dummy);
}
