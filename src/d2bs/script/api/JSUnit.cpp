#include "d2bs/script/api/JSUnit.h"

#include "d2bs/core/Core.h"
#include "d2bs/core/MPQStats.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Skills.h"
#include "d2bs/new_util/localization.h"
#include "d2bs/utils/CriticalSections.h"
#include "d2bs/utils/Helpers.h"

EMPTY_CTOR(unit)

void unit_finalize(JSFreeOp*, JSObject* obj) {
  Private* lpUnit = (Private*)JS_GetPrivate(obj);

  if (lpUnit) {
    switch (lpUnit->dwPrivateType) {
      case PRIVATE_UNIT: {
        myUnit* unit = (myUnit*)lpUnit;
        delete unit;
        break;
      }
      case PRIVATE_ITEM: {
        invUnit* unit = (invUnit*)lpUnit;
        delete unit;
        break;
      }
    }
  }
  JS_SetPrivate(obj, NULL);
}

JSAPI_PROP(unit_getProperty) {
  BnetData* pData = *p_D2LAUNCH_BnData;
  GameStructInfo* pInfo = *p_D2CLIENT_GameInfo;
  JS::Value ID;
  JS_IdToValue(cx, id, &ID);
  JS_BeginRequest(cx);

  switch (JSVAL_TO_INT(ID)) {
    case ME_PID:
      vp.setDouble((double)GetCurrentProcessId());
      // JS_NewNumberValue(cx, (double)GetCurrentProcessId(), vp);
      break;
    case ME_PROFILE:
      vp.setString(JS_NewUCStringCopyZ(cx, Vars.settings.szProfile));
      break;
    case ME_GAMEREADY:
      vp.setBoolean(GameReady());
      //*vp = BOOLEAN_TO_JSVAL(GameReady());
      break;
    case ME_ACCOUNT:
      if (!pData) return JS_TRUE;
      vp.setString(JS_NewStringCopyZ(cx, pData->szAccountName));
      // vp.setString(JS_NewStringCopyZ(cx, pData->szAccountName));
      break;
    case ME_CHARNAME:
      if (!pData) return JS_TRUE;
      vp.setString(JS_NewStringCopyZ(cx, pData->szPlayerName));
      break;
    case ME_CHICKENHP:
      vp.setInt32(Vars.nChickenHP);
      break;
    case ME_CHICKENMP:
      vp.setInt32(Vars.nChickenMP);
      break;
    case ME_DIFF:
      vp.setInt32(D2CLIENT_GetDifficulty());
      break;
    case ME_MAXDIFF:
      vp.setInt32(pData->nMaxDiff);
      break;
    case ME_GAMENAME:
      if (!pInfo) return JS_TRUE;
      vp.setString(JS_NewStringCopyZ(cx, pInfo->szGameName));
      break;
    case ME_GAMEPASSWORD:
      if (!pInfo) return JS_TRUE;
      vp.setString(JS_NewStringCopyZ(cx, pInfo->szGamePassword));
      break;
    case ME_GAMESERVERIP:
      if (!pInfo) return JS_TRUE;
      vp.setString(JS_NewStringCopyZ(cx, pInfo->szGameServerIp));
      break;
    case ME_GAMESTARTTIME:
      vp.setDouble((double)Vars.settings.dwGameTime);
      break;
    case ME_GAMETYPE:
      vp.setInt32(*p_D2CLIENT_ExpCharFlag);
      break;
    case ME_PLAYERTYPE:
      if (pData) vp.setBoolean(!!(pData->nCharFlags & PLAYER_TYPE_HARDCORE));
      break;
    case ME_ITEMONCURSOR:
      vp.setBoolean(!!(D2CLIENT_GetCursorItem()));
      break;
    case ME_AUTOMAP:
      vp.setBoolean(*p_D2CLIENT_AutomapOn);
      // JS_NewNumberValue(cx, (double)(*p_D2CLIENT_AutomapOn), vp);
      //*vp = *p_D2CLIENT_AutomapOn;
      break;
    case ME_LADDER:
      if (pData)
        // vp.setBoolean(!!(pData->ladderflag & (LADDERFLAG_SET|LADDERFLAG_EXPANSION_NORMAL)));
        vp.setDouble((double)pData->ladderflag);
      break;
    case ME_QUITONHOSTILE:
      vp.setBoolean(Vars.settings.bQuitOnHostile);
      break;
    case ME_REALM:
      vp.setString(JS_NewStringCopyZ(cx, pData->szRealmName));
      break;
    case ME_REALMSHORT:
      vp.setString(JS_NewStringCopyZ(cx, pData->szRealmName2));
      break;
    case OOG_SCREENSIZE:
      vp.setInt32(D2GFX_GetScreenSize());
      break;
    case OOG_WINDOWTITLE: {
      wchar_t szTitle[256];
      GetWindowTextW(D2GFX_GetHwnd(), szTitle, 256);
      vp.setString(JS_NewUCStringCopyZ(cx, szTitle));
    } break;
    case ME_PING:
      vp.setInt32(*p_D2CLIENT_Ping);
      break;
    case ME_FPS:
      vp.setInt32(*p_D2CLIENT_FPS);
      break;
    case ME_LOCALE:
      vp.setInt32(Vars.dwLocale);
      break;
    case OOG_INGAME:
      vp.setBoolean((ClientState() == ClientStateMenu ? false : true));
      break;
    case OOG_QUITONERROR:
      vp.setBoolean(Vars.settings.bQuitOnError);
      break;
    case OOG_MAXGAMETIME:
      vp.setInt32(Vars.settings.dwMaxGameTime);
      break;
    case ME_MERCREVIVECOST:
      vp.setInt32((*p_D2CLIENT_MercReviveCost));
      break;
    case ME_BLOCKKEYS:
      vp.setBoolean(Vars.bBlockKeys);
      break;
    case ME_BLOCKMOUSE:
      vp.setBoolean(Vars.bBlockMouse);
      break;
    case ME_UNSUPPORTED:
      vp.setBoolean(Vars.settings.bEnableUnsupported);
      break;
    case ME_CHARFLAGS:
      if (pData) vp.setInt32(pData->nCharFlags);
      break;
    default:
      break;
  }
  JS_EndRequest(cx);
  if (ClientState() != ClientStateInGame) return JS_TRUE;
  // JSObject* obj ;
  // JS_ValueToObject(cx,  JS_CALLEE(cx,vp),obj);
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(obj);
  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);
  if (!pUnit) {
    return JS_TRUE;
  }
  Room1* pRoom = NULL;

  switch (JSVAL_TO_INT(ID)) {
    case UNIT_TYPE:
      vp.setInt32(pUnit->dwType);
      break;
    case UNIT_CLASSID:
      vp.setInt32(pUnit->dwTxtFileNo);
      break;
    case UNIT_MODE:
      vp.setInt32(pUnit->dwMode);
      break;
    case UNIT_NAME: {
      char tmp[128] = "";
      GetUnitName(pUnit, tmp, 128);
      vp.setString(JS_InternString(cx, tmp));
    } break;
    case ME_MAPID:
      vp.setInt32(*p_D2CLIENT_MapId);
      break;
    case ME_NOPICKUP:
      vp.setBoolean(!!*p_D2CLIENT_NoPickUp);
      break;
    case UNIT_ACT:
      vp.setInt32(pUnit->dwAct + 1);
      break;
    case UNIT_AREA:
      pRoom = D2COMMON_GetRoomFromUnit(pUnit);
      if (pRoom && pRoom->pRoom2 && pRoom->pRoom2->pLevel) vp.setInt32(pRoom->pRoom2->pLevel->dwLevelNo);
      break;
    case UNIT_ID:
      JS_BeginRequest(cx);
      vp.setNumber((double)pUnit->dwUnitId);
      JS_EndRequest(cx);
      break;
    case UNIT_XPOS:
      vp.setInt32(D2CLIENT_GetUnitX(pUnit));
      break;
    case UNIT_YPOS:
      vp.setInt32(D2CLIENT_GetUnitY(pUnit));
      break;
    case UNIT_TARGETX:
      switch (pUnit->dwType) {
        case 0:
        case 1:
        case 3:
          vp.setInt32(pUnit->pPath->xTarget);
      }
      break;
    case UNIT_TARGETY:
      switch (pUnit->dwType) {
        case 0:
        case 1:
        case 3:
          vp.setInt32(pUnit->pPath->yTarget);
      }
      break;
    case UNIT_HP:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 6, 0) >> 8);
      break;
    case UNIT_HPMAX:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 7, 0) >> 8);
      break;
    case UNIT_MP:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 8, 0) >> 8);
      break;
    case UNIT_MPMAX:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 9, 0) >> 8);
      break;
    case UNIT_STAMINA:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 10, 0) >> 8);
      break;
    case UNIT_STAMINAMAX:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 11, 0) >> 8);
      break;
    case UNIT_CHARLVL:
      vp.setInt32(D2COMMON_GetUnitStat(pUnit, 12, 0));
      break;
    case ME_RUNWALK:
      if (pUnit == D2CLIENT_GetPlayerUnit()) vp.setInt32(*p_D2CLIENT_AlwaysRun);
      break;
    case UNIT_SPECTYPE:
      DWORD SpecType;
      SpecType = NULL;
      if (pUnit->dwType == UNIT_MONSTER && pUnit->pMonsterData) {
        if (pUnit->pMonsterData->fMinion & 1) SpecType |= 0x08;
        if (pUnit->pMonsterData->fBoss & 1) SpecType |= 0x04;
        if (pUnit->pMonsterData->fChamp & 1) SpecType |= 0x02;
        if ((pUnit->pMonsterData->fBoss & 1) && (pUnit->pMonsterData->fNormal & 1)) SpecType |= 0x01;
        if (pUnit->pMonsterData->fNormal & 1) SpecType |= 0x00;
        vp.setInt32(SpecType);
        return JS_TRUE;
      }
      break;
    case UNIT_UNIQUEID:
      if (pUnit->dwType == UNIT_MONSTER && pUnit->pMonsterData->fBoss && pUnit->pMonsterData->fNormal)
        vp.setInt32(pUnit->pMonsterData->wUniqueNo);
      else
        vp.setInt32(-1);
      break;
    case ITEM_CODE:  // replace with better method if found
      if (!(pUnit->dwType == UNIT_ITEM) && pUnit->pItemData) break;
      ItemTxt* pTxt;
      pTxt = D2COMMON_GetItemText(pUnit->dwTxtFileNo);
      if (!pTxt) {
        vp.setString(JS_InternString(cx, "Unknown"));
        return JS_TRUE;
      }
      char szCode[4];
      memcpy(szCode, pTxt->szCode, 3);
      szCode[3] = 0x00;
      vp.setString(JS_InternString(cx, szCode));
      break;
    case ITEM_PREFIX:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData)
        if (D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicPrefix[0]))
          vp.setString(JS_InternString(cx, D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicPrefix[0])));
      break;
    case ITEM_SUFFIX:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData)
        if (D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicSuffix[0]))
          vp.setString(JS_InternString(cx, D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicSuffix[0])));
      break;
    case ITEM_PREFIXNUM:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) vp.setInt32(pUnit->pItemData->wMagicPrefix[0]);
      break;
    case ITEM_SUFFIXNUM:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) vp.setInt32(pUnit->pItemData->wMagicSuffix[0]);
      break;

    case ITEM_PREFIXES:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);

        for (int i = 0; i < 3; i++) {
          if (D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicPrefix[i])) {
            JS::Value nPrefix =
                STRING_TO_JSVAL(JS_InternString(cx, D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicPrefix[i])));

            JS_SetElement(cx, pReturnArray, i, &nPrefix);
          }
        }
        vp.setObject(*pReturnArray);
        //*vp = OBJECT_TO_JSVAL(pReturnArray);
      }

      break;
    case ITEM_PREFIXNUMS:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);

        for (int i = 0; i < 3; i++) {
          if (pUnit->pItemData->wMagicPrefix[i]) {
            JS::Value nPrefixnum = INT_TO_JSVAL(pUnit->pItemData->wMagicPrefix[i]);

            JS_SetElement(cx, pReturnArray, i, &nPrefixnum);
          }
        }
        vp.setObject(*pReturnArray);
        //*vp = OBJECT_TO_JSVAL(pReturnArray);
      }

      break;
    case ITEM_SUFFIXES:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);

        for (int i = 0; i < 3; i++) {
          if (D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicSuffix[i])) {
            JS::Value nSuffix =
                STRING_TO_JSVAL(JS_InternString(cx, D2COMMON_GetItemMagicalMods(pUnit->pItemData->wMagicSuffix[i])));

            JS_SetElement(cx, pReturnArray, i, &nSuffix);
          }
        }
        vp.setObject(*pReturnArray);
        //*vp = OBJECT_TO_JSVAL(pReturnArray);
      }

      break;
    case ITEM_SUFFIXNUMS:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);

        for (int i = 0; i < 3; i++) {
          if (pUnit->pItemData->wMagicSuffix[i]) {
            JS::Value nSuffixnum = INT_TO_JSVAL(pUnit->pItemData->wMagicSuffix[i]);

            JS_SetElement(cx, pReturnArray, i, &nSuffixnum);
          }
        }

        vp.setObject(*pReturnArray);
      }

      break;

    case ITEM_FNAME:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        wchar_t wszfname[256] = L"";
        D2CLIENT_GetItemName(pUnit, wszfname, _countof(wszfname));
        if (wszfname) {
          vp.setString(JS_InternUCString(cx, wszfname));
        }
      }
      break;
    case ITEM_QUALITY:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) vp.setInt32(pUnit->pItemData->dwQuality);
      break;
    case ITEM_NODE:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) vp.setInt32(pUnit->pItemData->NodePage);
      break;
    case ITEM_LOC:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) vp.setInt32(pUnit->pItemData->GameLocation);
      break;
    case ITEM_SIZEX:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        if (!D2COMMON_GetItemText(pUnit->dwTxtFileNo)) break;
        vp.setInt32(D2COMMON_GetItemText(pUnit->dwTxtFileNo)->xSize);
      }
      break;
    case ITEM_SIZEY:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        if (!D2COMMON_GetItemText(pUnit->dwTxtFileNo)) break;
        vp.setInt32(D2COMMON_GetItemText(pUnit->dwTxtFileNo)->ySize);
      }
      break;
    case ITEM_TYPE:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) {
        if (!D2COMMON_GetItemText(pUnit->dwTxtFileNo)) break;
        vp.setInt32(D2COMMON_GetItemText(pUnit->dwTxtFileNo)->nType);
      }
      break;
    case ITEM_DESC: {
      if (pUnit->dwType != UNIT_ITEM) break;

      CriticalRoom cRoom;

      wchar_t wBuffer[2048] = L"";
      wchar_t bBuffer[1] = {1};
      if (pUnit->pItemData && pUnit->pItemData->pOwnerInventory && pUnit->pItemData->pOwnerInventory->pOwner) {
        ::WriteProcessMemory(GetCurrentProcess(), (void*)GetDllOffset("D2Client.dll", 0x7BCBE8 - 0x400000), bBuffer, 1,
                             NULL);
        ::WriteProcessMemory(GetCurrentProcess(), (void*)GetDllOffset("D2Client.dll", 0x7BCBF4 - 0x400000), &pUnit, 4,
                             NULL);

        // D2CLIENT_LoadItemDesc(D2CLIENT_GetPlayerUnit(), 0);
        D2CLIENT_LoadItemDesc(pUnit->pItemData->pOwnerInventory->pOwner, 0);
        ReadProcessBYTES(GetCurrentProcess(), GetDllOffset("D2Win.dll", 0x841EC8 - 0x400000), wBuffer, 2047);
      }
      if (wcslen(wBuffer) > 0) {
        vp.setString(JS_InternUCString(cx, wBuffer));
      }
    }

    break;
    case ITEM_GFX:
      if (pUnit->dwType == UNIT_ITEM && pUnit->pItemData) vp.setInt32(pUnit->pItemData->bInvGfxIdx);
      break;
    case UNIT_ITEMCOUNT:
      if (pUnit->pInventory) vp.setInt32(pUnit->pInventory->dwItemCount);
      break;
    case ITEM_BODYLOCATION:
      if (pUnit->dwType != UNIT_ITEM) break;
      if (pUnit->pItemData) vp.setInt32(pUnit->pItemData->BodyLocation);
      break;
    case UNIT_OWNER:
      vp.setNumber((double)pUnit->dwOwnerId);
      break;
    case UNIT_OWNERTYPE:
      vp.setInt32(pUnit->dwOwnerType);
      break;
    case ITEM_LEVEL:
      if (pUnit->dwType != UNIT_ITEM) break;
      if (pUnit->pItemData) vp.setInt32(pUnit->pItemData->dwItemLevel);
      break;
    case ITEM_LEVELREQ:
      if (pUnit->dwType != UNIT_ITEM) break;
      vp.setInt32(D2COMMON_GetItemLevelRequirement(pUnit, D2CLIENT_GetPlayerUnit()));
      break;
    case UNIT_DIRECTION:

      if (pUnit->pPath && pUnit->pPath->pRoom1) vp.setInt32(pUnit->pPath->bDirection);
      break;
    case OBJECT_TYPE:
      if (pUnit->dwType == UNIT_OBJECT && pUnit->pObjectData) {
        pRoom = D2COMMON_GetRoomFromUnit(pUnit);
        if (pRoom && D2COMMON_GetLevelNoFromRoom(pRoom))
          vp.setInt32(pUnit->pObjectData->Type & 255);
        else
          vp.setInt32(pUnit->pObjectData->Type);
      }
      break;
    case OBJECT_LOCKED:
      if (pUnit->dwType == UNIT_OBJECT && pUnit->pObjectData) vp.setInt32(pUnit->pObjectData->ChestLocked);
      break;
    case ME_WSWITCH:
      if (pUnit == D2CLIENT_GetPlayerUnit()) vp.setInt32(*p_D2CLIENT_bWeapSwitch);
      break;
    default:
      break;
  }

  return JS_TRUE;
}

JSAPI_STRICT_PROP(unit_setProperty) {
  JS::Value ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case ME_CHICKENHP:
      if (vp.isInt32()) Vars.nChickenHP = vp.toInt32();
      break;
    case ME_CHICKENMP:
      if (vp.isInt32()) Vars.nChickenMP = vp.toInt32();
      break;
    case ME_QUITONHOSTILE:
      if (vp.isBoolean()) Vars.settings.bQuitOnHostile = vp.toBoolean();
      break;
    case OOG_QUITONERROR:
      if (vp.isBoolean()) Vars.settings.bQuitOnError = vp.toBoolean();
      break;
    case OOG_MAXGAMETIME:
      if (vp.isInt32()) Vars.settings.dwMaxGameTime = vp.toInt32();
      break;
    case ME_BLOCKKEYS:
      if (vp.isBoolean()) Vars.bBlockKeys = vp.toBoolean();
      break;
    case ME_BLOCKMOUSE:
      if (vp.isBoolean()) Vars.bBlockMouse = vp.toBoolean();
      break;
    case ME_RUNWALK:
      *p_D2CLIENT_AlwaysRun = !!vp.toInt32();
      break;
    case ME_AUTOMAP:
      *p_D2CLIENT_AutomapOn = vp.toBoolean() ? 1 : 0;
      break;
    case ME_NOPICKUP:
      *p_D2CLIENT_NoPickUp = !!vp.toInt32();
      break;
  }
  return JS_TRUE;
}

JSAPI_FUNC(unit_getUnit) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS_SET_RVAL(cx, vp, JSVAL_VOID);

  if (argc < 1) return JS_TRUE;

  int nType = -1;
  uint32_t nClassId = (uint32_t)-1;
  uint32_t nMode = (uint32_t)-1;
  uint32_t nUnitId = (uint32_t)-1;
  char* szName = nullptr;

  {
    JSAutoRequest r(cx);
    if (args.get(0).isInt32()) nType = args[0].toInt32();
    if (args.get(1).isString()) szName = JS_EncodeStringToUTF8(cx, args[1].toString());
    if (args.get(1).isNumber() && !args.get(1).isNull()) JS_ValueToECMAUint32(cx, args[1], &nClassId);
    if (args.get(2).isNumber() && !args.get(2).isNull()) JS_ValueToECMAUint32(cx, args[2], &nMode);
    if (args.get(3).isNumber() && !args.get(3).isNull()) JS_ValueToECMAUint32(cx, args[3], &nUnitId);
  }

  UnitAny* pUnit = NULL;

  if (nType == 100)
    pUnit = D2CLIENT_GetCursorItem();
  else if (nType == 101) {
    pUnit = D2CLIENT_GetSelectedUnit();
    if (!pUnit) pUnit = (*p_D2CLIENT_SelectedInvItem);
  } else
    pUnit = GetUnit(szName, nClassId, nType, nMode, nUnitId);

  if (!pUnit) {
    JS_free(cx, szName);
    return JS_TRUE;
  }

  myUnit* pmyUnit = new myUnit;  // leaked?

  if (!pmyUnit) {
    JS_free(cx, szName);
    return JS_TRUE;
  }

  pmyUnit->_dwPrivateType = PRIVATE_UNIT;
  pmyUnit->dwClassId = nClassId;
  pmyUnit->dwMode = nMode;
  pmyUnit->dwType = pUnit->dwType;
  pmyUnit->dwUnitId = pUnit->dwUnitId;
  if (szName) {
    strcpy_s(pmyUnit->szName, sizeof(pmyUnit->szName), szName);
  }
  JS_free(cx, szName);

  JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);

  if (!jsunit) return JS_TRUE;

  args.rval().setObjectOrNull(jsunit);
  return JS_TRUE;
}

JSAPI_FUNC(unit_getNext) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  auto self = args.thisv().toObjectOrNull();
  Private* unit = (Private*)JS_GetPrivate(self);

  if (!unit) return JS_TRUE;

  if (unit->dwPrivateType == PRIVATE_UNIT) {
    myUnit* lpUnit = (myUnit*)unit;
    UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

    if (!pUnit) return JS_TRUE;

    JSAutoRequest r(cx);
    if (args.length() > 0 && args[0].isString()) {
      char* szText = JS_EncodeStringToUTF8(cx, args[0].toString());
      strcpy_s(lpUnit->szName, 128, szText);
      JS_free(cx, szText);
    }

    if (args.length() > 0 && args[0].isNumber() && !args[1].isNull())
      JS_ValueToECMAUint32(cx, args[0], (uint32_t*)&(lpUnit->dwClassId));

    if (args.length() > 1 && args[1].isNumber() && !args[2].isNull())
      JS_ValueToECMAUint32(cx, args[1], (uint32_t*)&(lpUnit->dwMode));

    pUnit = GetNextUnit(pUnit, lpUnit->szName, lpUnit->dwClassId, lpUnit->dwType, lpUnit->dwMode);

    if (!pUnit) {
      // Same thing as bobode's fix for finalize
      /*JSObject* obj = JS_THIS_OBJECT(cx, vp);
      //JS_ClearScope(cx, obj);
      if(JS_ValueToObject(cx, JSVAL_NULL, &obj) == JS_FALSE)
              return JS_TRUE;*/
      args.rval().setBoolean(false);
    } else {
      lpUnit->dwUnitId = pUnit->dwUnitId;
      JS_SetPrivate(self, lpUnit);
      args.rval().setBoolean(true);
    }
  } else if (unit->dwPrivateType == PRIVATE_ITEM) {
    invUnit* pmyUnit = (invUnit*)unit;
    if (!pmyUnit) return JS_TRUE;

    UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);
    UnitAny* pOwner = D2CLIENT_FindUnit(pmyUnit->dwOwnerId, pmyUnit->dwOwnerType);
    if (!pUnit || !pOwner) return JS_TRUE;

    JSAutoRequest r(cx);
    if (args.length() > 0 && args[0].isString()) {
      char* szText = JS_EncodeStringToUTF8(cx, args[0].toString());
      strcpy_s(pmyUnit->szName, 128, szText);
      JS_free(cx, szText);
    }

    if (args.length() > 0 && args[0].isNumber() && !args[1].isNull())
      JS_ValueToECMAUint32(cx, args[0], (uint32_t*)&(pmyUnit->dwClassId));

    if (args.length() > 1 && args[1].isNumber() && !args[2].isNull())
      JS_ValueToECMAUint32(cx, args[1], (uint32_t*)&(pmyUnit->dwMode));

    UnitAny* nextItem = GetInvNextUnit(pUnit, pOwner, pmyUnit->szName, pmyUnit->dwClassId, pmyUnit->dwMode);
    if (!nextItem) {
      // set current object to null breaks the unit_finilize cleanup cycle
      /*JSObject* obj = JS_THIS_OBJECT(cx, vp);
      //JS_ClearScope(cx, obj);
      if(JS_ValueToObject(cx, JSVAL_NULL, &obj) == JS_FALSE)
              return JS_TRUE;
      */
      args.rval().setBoolean(false);
    } else {
      pmyUnit->dwUnitId = nextItem->dwUnitId;
      JS_SetPrivate(self, pmyUnit);
      args.rval().setBoolean(true);
    }
  }

  return JS_TRUE;
}

JSAPI_FUNC(unit_cancel) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  DWORD automapOn = *p_D2CLIENT_AutomapOn;
  int32_t mode = -1;

  if (args.length() > 0) {
    JSAutoRequest r(cx);
    if (args[0].isNumber()) JS_ValueToECMAUint32(cx, args[0], (uint32_t*)&mode);
  } else if (IsScrollingText())
    mode = 3;
  else if (D2CLIENT_GetCurrentInteractingNPC())
    mode = 2;
  else if (D2CLIENT_GetCursorItem())
    mode = 1;
  else
    mode = 0;

  switch (mode) {
    case 0:
      D2CLIENT_CloseInteract();
      break;
    case 1:
      D2CLIENT_ClickMap(0, 10, 10, 0x08);
      break;
    case 2:
      D2CLIENT_CloseNPCInteract();
      break;
    case 3:
      D2CLIENT_ClearScreen();
      break;
  }

  *p_D2CLIENT_AutomapOn = automapOn;
  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(unit_repair) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);
  args.rval().setBoolean(false);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit) return JS_TRUE;

  BYTE aPacket[17] = {NULL};
  aPacket[0] = 0x35;
  *(DWORD*)&aPacket[1] = *p_D2CLIENT_RecentInteractId;
  aPacket[16] = 0x80;
  D2NET_SendPacket(17, 1, aPacket);

  // note: this crashes while minimized
  //	D2CLIENT_PerformNpcAction(pUnit,1, NULL);

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(unit_useMenu) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);
  JS_SET_RVAL(cx, vp, JSVAL_FALSE);

  if (args.length() < 1 || !args[0].isInt32()) return JS_TRUE;

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit) return JS_TRUE;

  args.rval().setBoolean(ClickNPCMenu(pUnit->dwTxtFileNo, args[0].toInt32()));
  return JS_TRUE;
}

JSAPI_FUNC(unit_interact) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  args.rval().setBoolean(false);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit || pUnit == D2CLIENT_GetPlayerUnit()) return JS_TRUE;

  if (pUnit->dwType == UNIT_ITEM && pUnit->dwMode != ITEM_MODE_ON_GROUND && pUnit->dwMode != ITEM_MODE_BEING_DROPPED) {
    int nLocation = GetItemLocation(pUnit);

    BYTE aPacket[13] = {NULL};

    if (nLocation == LOCATION_INVENTORY || nLocation == LOCATION_STASH) {
      aPacket[0] = 0x20;
      *(DWORD*)&aPacket[1] = pUnit->dwUnitId;
      *(DWORD*)&aPacket[5] = D2CLIENT_GetPlayerUnit()->pPath->xPos;
      *(DWORD*)&aPacket[9] = D2CLIENT_GetPlayerUnit()->pPath->yPos;
      D2NET_SendPacket(13, 1, aPacket);
      return JS_TRUE;
    } else if (nLocation == LOCATION_BELT) {
      aPacket[0] = 0x26;
      *(DWORD*)&aPacket[1] = pUnit->dwUnitId;
      *(DWORD*)&aPacket[5] = 0;
      *(DWORD*)&aPacket[9] = 0;
      D2NET_SendPacket(13, 1, aPacket);
      return JS_TRUE;
    }
  }

  if (pUnit->dwType == UNIT_OBJECT && args.length() == 1 && args[0].isInt32()) {
    // TODO: check the range on argv[0] to make sure it won't crash the game - Done! TechnoHunter
    int32_t nWaypointID;
    JSAutoRequest r(cx);
    if (!JS_ValueToECMAInt32(cx, args[0], &nWaypointID)) {
      return JS_TRUE;
    }

    int retVal = 0;
    if (FillBaseStat("levels", nWaypointID, "Waypoint", &retVal, sizeof(int)))
      if (retVal == 255) return JS_TRUE;

    D2CLIENT_TakeWaypoint(pUnit->dwUnitId, nWaypointID);
    if (!D2CLIENT_GetUIState(UI_GAME)) D2CLIENT_CloseInteract();

    args.rval().setBoolean(true);
    return JS_TRUE;
  }
  //	else if(pUnit->dwType == UNIT_PLAYER && argc == 1 && JSVAL_IS_INT(argv[0]) && JSVAL_TO_INT(argv[0]) == 1)
  //	{
  // Accept Trade
  //	}
  else {
    args.rval().setBoolean(true);
    ClickMap(0, D2CLIENT_GetUnitX(pUnit), D2CLIENT_GetUnitY(pUnit), FALSE, pUnit);
  }

  return JS_TRUE;
}

void InsertStatsToGenericObject(UnitAny* pUnit, StatList* pStatList, JSContext* pJSContext, JSObject* pGenericObject);
void InsertStatsNow(Stat* pStat, int nStat, JSContext* cx, JSObject* pArray);

JSAPI_FUNC(unit_getStat) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  args.rval().setBoolean(false);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit) return JS_TRUE;

  int32_t nStat = 0;
  int32_t nSubIndex = 0;
  {
    JSAutoRequest r(cx);
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "i/i", &nStat, &nSubIndex)) {
      return JS_TRUE;
    }
  }

  if (nStat >= STAT_HP && nStat <= STAT_MAXSTAMINA) {
    args.rval().setInt32(D2COMMON_GetUnitStat(pUnit, nStat, nSubIndex) >> 8);
  } else if (nStat == STAT_EXP || nStat == STAT_LASTEXP || nStat == STAT_NEXTEXP) {
    args.rval().setInt32(D2COMMON_GetUnitStat(pUnit, nStat, nSubIndex));
  } else if (nStat == STAT_ITEMLEVELREQ) {
    args.rval().setInt32(D2COMMON_GetItemLevelRequirement(pUnit, D2CLIENT_GetPlayerUnit()));
  } else if (nStat == -1) {
    Stat aStatList[256] = {NULL};
    StatList* pStatList = D2COMMON_GetStatList(pUnit, NULL, 0x40);

    if (pStatList) {
      DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);

      JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);
      args.rval().setObjectOrNull(pReturnArray);

      for (int j = 0; j < pUnit->pStats->StatVec.wCount; j++) {
        bool inListAlready = false;
        for (DWORD k = 0; k < dwStats; k++) {
          if (aStatList[k].dwStatValue == pUnit->pStats->StatVec.pStats[j].dwStatValue &&
              aStatList[k].wStatIndex == pUnit->pStats->StatVec.pStats[j].wStatIndex &&
              aStatList[k].wSubIndex == pUnit->pStats->StatVec.pStats[j].wSubIndex)
            inListAlready = true;
        }
        if (!inListAlready) {
          aStatList[dwStats].dwStatValue = pUnit->pStats->StatVec.pStats[j].dwStatValue;
          aStatList[dwStats].wStatIndex = pUnit->pStats->StatVec.pStats[j].wStatIndex;
          aStatList[dwStats].wSubIndex = pUnit->pStats->StatVec.pStats[j].wSubIndex;
          dwStats++;
        }
      }
      for (UINT i = 0; i < dwStats; i++) {
        JSAutoRequest r(cx);
        JS::RootedObject pArrayInsert(cx, JS_NewArrayObject(cx, 0, NULL));

        if (!pArrayInsert) continue;

        JS::Value nIndex = JS::Int32Value(aStatList[i].wStatIndex);
        JS::Value nSubIndexVal = JS::Int32Value(aStatList[i].wSubIndex);
        JS::Value nValue = JS::Int32Value(aStatList[i].dwStatValue);

        JS_SetElement(cx, pArrayInsert, 0, &nIndex);
        JS_SetElement(cx, pArrayInsert, 1, &nSubIndexVal);
        JS_SetElement(cx, pArrayInsert, 2, &nValue);

        JS::Value aObj = OBJECT_TO_JSVAL(pArrayInsert);

        JS_SetElement(cx, pReturnArray, i, &aObj);
      }
    }
  } else if (nStat == -2) {
    JSObject* pArray = JS_NewArrayObject(cx, 0, NULL);
    args.rval().setObjectOrNull(pArray);

    InsertStatsToGenericObject(pUnit, pUnit->pStats, cx, pArray);
    InsertStatsToGenericObject(pUnit, D2COMMON_GetStatList(pUnit, NULL, 0x40), cx, pArray);
    // InsertStatsToGenericObject(pUnit, pUnit->pStats->pNext, cx, pArray);  // only check the current unit stats!
    //	InsertStatsToGenericObject(pUnit, pUnit->pStats->pSetList, cx, pArray);
  } else {
    long result = D2COMMON_GetUnitStat(pUnit, nStat, nSubIndex);
    if (result == 0)  // if stat isnt found look up preset list
    {
      StatList* pStatList = D2COMMON_GetStatList(pUnit, NULL, 0x40);
      Stat aStatList[256] = {NULL};
      if (pStatList) {
        DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
        for (UINT i = 0; i < dwStats; i++) {
          if (nStat == aStatList[i].wStatIndex && nSubIndex == aStatList[i].wSubIndex)
            result = (aStatList[i].dwStatValue);
        }
      }
    }
    args.rval().setNumber(static_cast<double>(result));
  }
  return JS_TRUE;
}

void InsertStatsToGenericObject(UnitAny* pUnit, StatList* pStatList, JSContext* cx, JSObject* pArray) {
  Stat* pStat = NULL;
  if (!pStatList) return;
  if ((pStatList->dwUnitId == pUnit->dwUnitId && pStatList->dwUnitType == pUnit->dwType) || pStatList->pUnit == pUnit) {
    pStat = pStatList->StatVec.pStats;

    for (int nStat = 0; nStat < pStatList->StatVec.wCount; nStat++) {
      InsertStatsNow(pStat, nStat, cx, pArray);
    }
  }
  if ((pStatList->dwFlags >> 24 & 0x80)) {
    pStat = pStatList->SetStatVec.pStats;

    for (int nStat = 0; nStat < pStatList->SetStatVec.wCount; nStat++) {
      InsertStatsNow(pStat, nStat, cx, pArray);
    }
  }
}

void InsertStatsNow(Stat* pStat, int nStat, JSContext* cx, JSObject* pArray) {
  if (pStat[nStat].wSubIndex > 0x200) {
    // subindex is the skill id and level
    int skill = pStat[nStat].wSubIndex >> 6, level = pStat[nStat].wSubIndex & 0x3F, charges = 0, maxcharges = 0;
    if (pStat[nStat].dwStatValue > 0x200) {
      charges = pStat[nStat].dwStatValue & 0xFF;
      maxcharges = pStat[nStat].dwStatValue >> 8;
    }
    JSObject* val = BuildObject(cx, NULL);
    JS::Value jsskill = JS::Int32Value(skill), jslevel = JS::Int32Value(level), jscharges = JS::Int32Value(charges),
              jsmaxcharges = JS::Int32Value(maxcharges);
    // val is an anonymous object that holds properties
    if (!JS_SetProperty(cx, val, "skill", &jsskill) || !JS_SetProperty(cx, val, "level", &jslevel)) return;
    if (maxcharges > 0) {
      if (!JS_SetProperty(cx, val, "charges", &jscharges) || !JS_SetProperty(cx, val, "maxcharges", &jsmaxcharges))
        return;
    }
    // find where we should put it
    JS::Value index = JS::UndefinedValue(), obj = JS::ObjectOrNullValue(val);
    if (!JS_GetElement(cx, pArray, pStat[nStat].wStatIndex, &index)) return;
    if (index != JS::UndefinedValue()) {
      // modify the existing object by stuffing it into an array
      if (!JS_IsArrayObject(cx, index.toObjectOrNull())) {
        // it's not an array, build one
        JSAutoRequest r(cx);
        JS::RootedObject arr(cx, JS_NewArrayObject(cx, 0, NULL));
        JS_SetElement(cx, arr, 0, &index);
        JS_SetElement(cx, arr, 1, &obj);
        JS::Value arr2 = JS::ObjectOrNullValue(arr);
        JS_SetElement(cx, pArray, pStat[nStat].wStatIndex, &arr2);
      } else {
        // it is an array, append the new value
        JSObject* arr = index.toObjectOrNull();
        uint32_t len = 0;
        if (!JS_GetArrayLength(cx, arr, &len)) return;
        len++;
        JSAutoRequest r(cx);
        JS_SetElement(cx, arr, len, &obj);
      }
    } else {
      JSAutoRequest r(cx);
      JS_SetElement(cx, pArray, pStat[nStat].wStatIndex, &obj);
    }
  } else {
    // Make sure to bit shift life, mana and stamina properly!
    int value = pStat[nStat].dwStatValue;
    if (pStat[nStat].wStatIndex >= 6 && pStat[nStat].wStatIndex <= 11) value = value >> 8;

    JS::Value index = JS::UndefinedValue(), val = JS::Int32Value(value);
    if (!JS_GetElement(cx, pArray, pStat[nStat].wStatIndex, &index)) return;
    if (index == JS::UndefinedValue()) {
      // the array index doesn't exist, make it
      index = JS::ObjectOrNullValue(JS_NewArrayObject(cx, 0, NULL));
      JSAutoRequest r(cx);
      if (!JS_SetElement(cx, pArray, pStat[nStat].wStatIndex, &index)) {
        return;
      }
    }
    // index now points to the correct array index
    JSAutoRequest r(cx);
    JS_SetElement(cx, JSVAL_TO_OBJECT(index), pStat[nStat].wSubIndex, &val);
  }
}

JSAPI_FUNC(unit_getState) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  args.rval().setBoolean(false);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit || !args.get(0).isInt32()) return JS_TRUE;

  int32_t nState;
  JSAutoRequest r(cx);
  if (JS_ValueToInt32(cx, args[0], &nState) == JS_FALSE) {
    return JS_TRUE;
  }

  // TODO: make these constants so we know what we're checking here
  if (nState > 183 || nState < 0) return JS_TRUE;

  args.rval().setBoolean(D2COMMON_GetUnitState(pUnit, nState));
  return JS_TRUE;
}

JSAPI_FUNC(item_getFlags) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit || pUnit->dwType != UNIT_ITEM) return JS_TRUE;

  args.rval().setInt32(pUnit->pItemData->dwFlags);
  return JS_TRUE;
}

JSAPI_FUNC(item_getFlag) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (!args.get(0).isInt32()) return JS_TRUE;

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit || pUnit->dwType != UNIT_ITEM) return JS_TRUE;

  int32_t nFlag = args[0].toInt32();

  args.rval().setBoolean(!!(nFlag & pUnit->pItemData->dwFlags));
  return JS_TRUE;
}

// JSAPI_FUNC(item_getPrice)
//{
//	DEPRECATED;
//
//	if(!WaitForGameReady())
//		THROW_WARNING(cx, vp,  "Game not ready");
//
//	int diff = D2CLIENT_GetDifficulty();
//	//D2COMMON_GetItemPrice(D2CLIENT_GetPlayerUnit(), pUnit, diff, *p_D2CLIENT_ItemPriceList, NPCID, buysell)
//	int buysell = 0;
//	int NPCID = 148;
//
//	myUnit* lpUnit = (myUnit*)JS_GetPrivate(JS_THIS_OBJECT(cx, vp));
//
//	if(!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT)
//		return JS_TRUE;
//
//	UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);
//
//	if(!pUnit)
//		return JS_TRUE;
//
//	if(argc>0)
//	{
//		if(JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[0]))
//		{
//			myUnit* pmyNpc = (myUnit*)JS_GetPrivate(JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[0]));
//
//			if(!pmyNpc || (pmyNpc->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT)
//				return JS_TRUE;
//
//			UnitAny* pNpc = D2CLIENT_FindUnit(pmyNpc->dwUnitId, pmyNpc->dwType);
//
//			if(!pNpc)
//				return JS_TRUE;
//
//			NPCID = pNpc->dwTxtFileNo;
//		}
//		else if(JSVAL_IS_INT(JS_ARGV(cx, vp)[0]))
//			NPCID = JSVAL_TO_INT(JS_ARGV(cx, vp)[0]);
//	}
//	if(argc>1)
//		buysell = JSVAL_TO_INT(JS_ARGV(cx, vp)[1]);
//	if(argc>2)
//		diff = JSVAL_TO_INT(JS_ARGV(cx, vp)[2]);
//
//	*rval = INT_TO_JSVAL(D2COMMON_GetItemPrice(D2CLIENT_GetPlayerUnit(), pUnit, diff, *p_D2CLIENT_ItemPriceList,
// NPCID, buysell));
//
//	return JS_TRUE;
//}

JSAPI_FUNC(item_getItemCost) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  int32_t nMode;
  UnitAny* npc = D2CLIENT_GetCurrentInteractingNPC();
  int32_t nNpcClassId = (npc ? npc->dwTxtFileNo : 0x9A);  // defaults to Charsi's NPC id
  int32_t nDifficulty = D2CLIENT_GetDifficulty();

  if (!args.get(0).isInt32()) return JS_TRUE;

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit || pUnit->dwType != UNIT_ITEM) return JS_TRUE;

  nMode = args[0].toInt32();
  if (args.length() > 1) {
    if (args[1].isObject()) {
      myUnit* pmyNpc = (myUnit*)JS_GetPrivate(args[1].toObjectOrNull());

      if (!pmyNpc || (pmyNpc->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

      UnitAny* pNpc = D2CLIENT_FindUnit(pmyNpc->dwUnitId, pmyNpc->dwType);

      if (!pNpc) return JS_TRUE;
      nNpcClassId = pNpc->dwTxtFileNo;
    } else if (args[1].isInt32() && !args[1].isNull()) {
      JSAutoRequest r(cx);
      if (!JS_ValueToECMAInt32(cx, args[1], &nNpcClassId)) {
        return JS_TRUE;
      }
    }
    // TODO:: validate the base stat table sizes to make sure the game doesn't crash with checking values past the end
    // of the table
    int retVal = 0;
    if (FillBaseStat("monstats", nNpcClassId, "inventory", &retVal, sizeof(int)) && retVal == 0)
      nNpcClassId = 0x9A;  // invalid npcid incoming! default to charsi to allow the game to continue
  }

  if (args.get(2).isInt32()) nDifficulty = args[2].toInt32();

  switch (nMode) {
    case 0:  // Buy
    case 1:  // Sell
      args.rval().setInt32(D2COMMON_GetItemPrice(D2CLIENT_GetPlayerUnit(), pUnit, nDifficulty,
                                                 *p_D2CLIENT_ItemPriceList, nNpcClassId, nMode));
      break;
    case 2:  // Repair
      args.rval().setInt32(D2COMMON_GetItemPrice(D2CLIENT_GetPlayerUnit(), pUnit, nDifficulty,
                                                 *p_D2CLIENT_ItemPriceList, nNpcClassId, 3));
      break;
    default:
      break;
  }

  return JS_TRUE;
}

JSAPI_FUNC(unit_getItems) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit || !pUnit->pInventory || !pUnit->pInventory->pFirstItem) return JS_TRUE;

  JSAutoRequest r(cx);
  JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));

  if (!pReturnArray) {
    return JS_TRUE;
  }

  DWORD dwArrayCount = 0;

  for (UnitAny* pItem = pUnit->pInventory->pFirstItem; pItem;
       pItem = D2COMMON_GetNextItemFromInventory(pItem), dwArrayCount++) {
    invUnit* pmyUnit = new invUnit;

    if (!pmyUnit) continue;

    pmyUnit->_dwPrivateType = PRIVATE_UNIT;
    pmyUnit->szName[0] = NULL;
    pmyUnit->dwMode = pItem->dwMode;
    pmyUnit->dwClassId = pItem->dwTxtFileNo;
    pmyUnit->dwUnitId = pItem->dwUnitId;
    pmyUnit->dwType = UNIT_ITEM;
    pmyUnit->dwOwnerId = pUnit->dwUnitId;
    pmyUnit->dwOwnerType = pUnit->dwType;

    JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);
    if (!jsunit) {
      THROW_ERROR(cx, "Failed to build item array");
    }
    JS::Value a = JS::ObjectOrNullValue(jsunit);
    JS_SetElement(cx, pReturnArray, dwArrayCount, &a);
  }

  args.rval().setObjectOrNull(pReturnArray);
  return JS_TRUE;
}

JSAPI_FUNC(unit_getSkill) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (argc == NULL || argc > 3) return JS_TRUE;

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  bool nCharge = false;
  int32_t nSkillId = NULL;
  int32_t nExt = NULL;

  auto self = args.thisv().toObjectOrNull();
  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(self);
  if (!pmyUnit) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);
  if (!pUnit) return JS_TRUE;

  if (args.length() >= 1) {
    if (!args[0].isInt32()) return JS_TRUE;

    nSkillId = args[0].toInt32();
  }

  if (args.length() >= 2) {
    if (!args[1].isInt32()) return JS_TRUE;

    nExt = args[1].toInt32();
  }

  if (args.length() == 3) {
    if (!args[2].isBoolean()) return JS_TRUE;

    nCharge = args[2].toBoolean();
  }

  if (args.length() == 1) {
    WORD wLeftSkillId = pUnit->pInfo->pLeftSkill->pSkillInfo->wSkillId;
    WORD wRightSkillId = pUnit->pInfo->pRightSkill->pSkillInfo->wSkillId;
    switch (nSkillId) {
      case 0: {
        int row = 0;
        if (FillBaseStat("skills", wRightSkillId, "skilldesc", &row, sizeof(int)))
          if (FillBaseStat("skilldesc", row, "str name", &row, sizeof(int))) {
            wchar_t* szName = D2LANG_GetLocaleText((WORD)row);
            args.rval().setString(JS_NewUCStringCopyZ(cx, szName));
          }
      } break;
      case 1: {
        int row = 0;
        if (FillBaseStat("skills", wLeftSkillId, "skilldesc", &row, sizeof(int)))
          if (FillBaseStat("skilldesc", row, "str name", &row, sizeof(int))) {
            wchar_t* szName = D2LANG_GetLocaleText((WORD)row);
            args.rval().setString(JS_NewUCStringCopyZ(cx, szName));
          }
      } break;

      case 2:
        args.rval().setInt32(wRightSkillId);
        break;
      case 3:
        args.rval().setInt32(wLeftSkillId);
        break;
      case 4: {
        JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);
        args.rval().setObjectOrNull(pReturnArray);
        int i = 0;
        for (Skill* pSkill = pUnit->pInfo->pFirstSkill; pSkill; pSkill = pSkill->pNextSkill) {
          JS::RootedObject pArrayInsert(cx, JS_NewArrayObject(cx, 0, NULL));

          if (!pArrayInsert) continue;

          JS::Value nId = INT_TO_JSVAL(pSkill->pSkillInfo->wSkillId);
          JS::Value nBase = INT_TO_JSVAL(pSkill->dwSkillLevel);
          JS::Value nTotal = INT_TO_JSVAL(D2COMMON_GetSkillLevel(pUnit, pSkill, 1));
          JSAutoRequest r(cx);
          JS_SetElement(cx, pArrayInsert, 0, &nId);
          JS_SetElement(cx, pArrayInsert, 1, &nBase);
          JS_SetElement(cx, pArrayInsert, 2, &nTotal);

          JS::Value aObj = JS::ObjectOrNullValue(pArrayInsert);

          JS_SetElement(cx, pReturnArray, i, &aObj);
          i++;
        }
        break;
      }
      default:
        args.rval().setBoolean(false);
        break;
    }
    return JS_TRUE;
  }

  if (pUnit && pUnit->pInfo && pUnit->pInfo->pFirstSkill) {
    for (Skill* pSkill = pUnit->pInfo->pFirstSkill; pSkill; pSkill = pSkill->pNextSkill) {
      if (pSkill->pSkillInfo && pSkill->pSkillInfo->wSkillId == nSkillId) {
        if ((argc == 2 && pSkill->IsCharge == 0) || (argc == 3 && (nCharge == false || pSkill->IsCharge == 1))) {
          args.rval().setInt32(D2COMMON_GetSkillLevel(pUnit, pSkill, nExt));
          return JS_TRUE;
        }
      }
    }
  }

  args.rval().setBoolean(false);
  return JS_TRUE;
}

JSAPI_FUNC(item_shop) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setBoolean(false);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  CriticalRoom cRoom;

  if (*p_D2CLIENT_TransactionDialog != 0 || *p_D2CLIENT_TransactionDialogs != 0 ||
      *p_D2CLIENT_TransactionDialogs_2 != 0) {
    return JS_TRUE;
  }

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpItem = (myUnit*)JS_GetPrivate(self);

  if (!lpItem || (lpItem->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) {
    return JS_TRUE;
  }

  UnitAny* pItem = D2CLIENT_FindUnit(lpItem->dwUnitId, lpItem->dwType);

  if (!pItem || pItem->dwType != UNIT_ITEM) {
    return JS_TRUE;
  }

  if (!D2CLIENT_GetUIState(UI_NPCSHOP)) {
    return JS_TRUE;
  }

  UnitAny* pNPC = D2CLIENT_GetCurrentInteractingNPC();
  DWORD dwMode = args[argc - 1].toInt32();

  // Check if we are interacted.
  if (!pNPC) {
    return JS_TRUE;
  }

  // Check for proper mode.
  if ((dwMode != 1) && (dwMode != 2) && (dwMode != 6)) {
    return JS_TRUE;
  }

  // Selling an Item
  if (dwMode == 1) {
    // Check if we own the item!
    if (pItem->pItemData->pOwnerInventory->pOwner->dwUnitId != D2CLIENT_GetPlayerUnit()->dwUnitId) {
      return JS_TRUE;
    }

    D2CLIENT_ShopAction(pNPC, pItem, 1, 0, 0, 1, 1, NULL);
  } else {
    // Make sure the item is owned by the NPC interacted with.
    if (pItem->pItemData->pOwnerInventory->pOwner->dwUnitId != pNPC->dwUnitId) {
      return JS_TRUE;
    }

    D2CLIENT_ShopAction(pNPC, pItem, 0, 0, 0, dwMode, 1, NULL);
  }

  /*BYTE pPacket[17] = {NULL};

  if(dwMode == 2 || dwMode == 6)
          pPacket[0] = 0x32;
  else pPacket[0] = 0x33;

  *(DWORD*)&pPacket[1] = pNPC->dwUnitId;
  *(DWORD*)&pPacket[5] = pItem->dwUnitId;

  if(dwMode == 1) // Sell
  {
          if(D2CLIENT_GetCursorItem() && D2CLIENT_GetCursorItem() == pItem)
                  *(DWORD*)&pPacket[9] = 0x04;
          else
                  *(DWORD*)&pPacket[9] = 0;
  }
  else if(dwMode == 2) // Buy
  {
          if(pItem->pItemData->dwFlags & 0x10)
                  *(DWORD*)&pPacket[9] = 0x00;
          else
                  *(DWORD*)&pPacket[9] = 0x02;
  }
  else
          *(BYTE*)&pPacket[9+3] = 0x80;

  int nBuySell = NULL;

  if(dwMode == 2 || dwMode == 6)
          nBuySell = NULL;
  else nBuySell = 1;

  *(DWORD*)&pPacket[13] = D2COMMON_GetItemPrice(D2CLIENT_GetPlayerUnit(), pItem, D2CLIENT_GetDifficulty(),
  *p_D2CLIENT_ItemPriceList, pNPC->dwTxtFileNo, nBuySell);

  D2NET_SendPacket(sizeof(pPacket), 1, pPacket);*/

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(unit_getParent) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  if (!lpUnit || (lpUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);

  if (!pUnit) return JS_TRUE;

  if (pUnit->dwType == UNIT_MONSTER) {
    DWORD dwOwnerId = D2CLIENT_GetMonsterOwner(pUnit->dwUnitId);
    if (dwOwnerId == -1) return JS_TRUE;

    UnitAny* pMonster = GetUnit(NULL, (DWORD)-1, (DWORD)-1, (DWORD)-1, dwOwnerId);
    if (!pMonster) return JS_TRUE;

    myUnit* pmyUnit = new myUnit;
    if (!pmyUnit) return JS_TRUE;

    pmyUnit->_dwPrivateType = PRIVATE_UNIT;
    pmyUnit->dwUnitId = pMonster->dwUnitId;
    pmyUnit->dwClassId = pMonster->dwTxtFileNo;
    pmyUnit->dwMode = pMonster->dwMode;
    pmyUnit->dwType = pMonster->dwType;
    pmyUnit->szName[0] = NULL;

    JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);
    if (!jsunit) return JS_TRUE;
    args.rval().setObjectOrNull(jsunit);
    return JS_TRUE;
  } else if (pUnit->dwType == UNIT_OBJECT) {
    if (pUnit->pObjectData) {
      char szBuffer[128] = "";
      strcpy_s(szBuffer, sizeof(szBuffer), pUnit->pObjectData->szOwner);

      args.rval().setString(JS_InternString(cx, szBuffer));
    }
  } else if (pUnit->dwType == UNIT_ITEM) {
    if (pUnit->pItemData && pUnit->pItemData->pOwnerInventory && pUnit->pItemData->pOwnerInventory->pOwner) {
      myUnit* pmyUnit = new myUnit;  // leaks

      if (!pmyUnit) return JS_TRUE;

      pmyUnit->_dwPrivateType = PRIVATE_UNIT;
      pmyUnit->dwUnitId = pUnit->pItemData->pOwnerInventory->pOwner->dwUnitId;
      pmyUnit->dwClassId = pUnit->pItemData->pOwnerInventory->pOwner->dwTxtFileNo;
      pmyUnit->dwMode = pUnit->pItemData->pOwnerInventory->pOwner->dwMode;
      pmyUnit->dwType = pUnit->pItemData->pOwnerInventory->pOwner->dwType;
      pmyUnit->szName[0] = NULL;
      JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);

      args.rval().setObjectOrNull(jsunit);
    }
  } else if (pUnit->dwType == UNIT_MISSILE) {
    auto* pmyUnit = new myUnit;
    if (!pmyUnit) return JS_TRUE;

    UnitAny* pOwner = D2COMMON_GetMissileOwnerUnit(pUnit);
    if (!pOwner) return JS_TRUE;

    pmyUnit->_dwPrivateType = PRIVATE_UNIT;
    pmyUnit->dwUnitId = pOwner->dwUnitId;
    pmyUnit->dwClassId = pOwner->dwTxtFileNo;
    pmyUnit->dwMode = pOwner->dwMode;
    pmyUnit->dwType = pOwner->dwType;
    pmyUnit->szName[0] = NULL;

    JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);
    args.rval().setObjectOrNull(jsunit);
  }

  return JS_TRUE;
}

// Works only on players sinces monsters _CANT_ have mercs!
JSAPI_FUNC(unit_getMerc) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNull();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* lpUnit = (myUnit*)JS_GetPrivate(self);

  if (lpUnit && (lpUnit->_dwPrivateType & PRIVATE_UNIT) == PRIVATE_UNIT) {
    UnitAny* pUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);
    if (pUnit && pUnit->dwType == UNIT_PLAYER) {
      UnitAny* pMerc = GetMercUnit(pUnit);
      if (pMerc) {
        myUnit* pmyUnit = new myUnit;

        pmyUnit->_dwPrivateType = PRIVATE_UNIT;
        pmyUnit->dwUnitId = pMerc->dwUnitId;
        pmyUnit->dwClassId = pMerc->dwTxtFileNo;
        pmyUnit->dwMode = NULL;
        pmyUnit->dwType = UNIT_MONSTER;
        pmyUnit->szName[0] = NULL;

        JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);
        if (jsunit) {
          args.rval().setObjectOrNull(jsunit);
        }
      }
    }
  }
  return JS_TRUE;
}

JSAPI_FUNC(unit_getMercHP) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  JS::Value* rest = 0;
  myUnit* lpUnit = (myUnit*)JS_GetInstancePrivate(cx, self, &unit_class, rest);
  // myUnit* lpUnit = (myUnit*)JS_GetPrivate(test);

  UnitAny* pUnit = lpUnit ? D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType) : D2CLIENT_GetPlayerUnit();

  if (pUnit) {
    UnitAny* pMerc = GetMercUnit(pUnit);
    if (pMerc)
      JS_SET_RVAL(cx, vp,
                  (pUnit->dwMode == 12 ? JSVAL_ZERO : INT_TO_JSVAL(D2CLIENT_GetUnitHPPercent(pMerc->dwUnitId))));
  }

  return JS_TRUE;
}

// unit.setSkill( int skillId OR String skillName, int hand [, int itemGlobalId] );
JSAPI_FUNC(unit_setskill) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setBoolean(false);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  WORD nSkillId = (WORD)-1;
  BOOL nHand = FALSE;
  DWORD itemId = (DWORD)-1;

  if (args.length() < 1) return JS_TRUE;

  if (args[0].isString()) {
    char* name = JS_EncodeStringToUTF8(cx, args[0].toString());
    nSkillId = GetSkillByName(name);
    JS_free(cx, name);
  } else if (args[0].isInt32())
    nSkillId = (WORD)args[0].toInt32();
  else
    return JS_TRUE;

  if (args[1].isInt32())
    nHand = !!args[1].toInt32();
  else
    return JS_TRUE;

  if (args.length() == 3 && args[2].isObject()) {
    JSObject* object = args[2].toObjectOrNull();
    if (JS_InstanceOf(cx, object, &unit_class, args.array())) {
      myUnit* unit = (myUnit*)JS_GetPrivate(object);
      if (unit->dwType == UNIT_ITEM) itemId = unit->dwUnitId;
    }
  }

  if (SetSkill(cx, nSkillId, nHand, itemId)) {
    args.rval().setBoolean(true);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_overhead) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setBoolean(false);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(self);

  if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

  if (!pUnit) return JS_TRUE;

  if (!args[0].isNullOrUndefined()) {
    const wchar_t* lpszText = JS_GetStringCharsZ(cx, args[0].toString());
    if (lpszText && lpszText[0]) {
      // Fix overhead msg for locale text
      // wchar_t* tmpw = AnsiToUnicode(lpszText);
      // Convert back to multibyte in locale code page
      auto tmpc = d2bs::util::wide_to_ansi(lpszText);
      OverheadMsg* pMsg = D2COMMON_GenerateOverheadMsg(NULL, tmpc.data(), *p_D2CLIENT_OverheadTrigger);
      if (pMsg) {
        // D2COMMON_FixOverheadMsg(pMsg, NULL);
        pUnit->pOMsg = pMsg;
      }
      // delete[] tmpw;
    }
    // JS_free(cx, lpszText);
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_revive) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  BYTE pPacket[] = {0x41};
  D2NET_SendPacket(1, 1, pPacket);
  return JS_TRUE;
}

JSAPI_FUNC(unit_getItem) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(self);

  if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

  if (!pUnit || !pUnit->pInventory) return JS_TRUE;

  uint32_t nClassId = (uint32_t)-1;
  uint32_t nMode = (uint32_t)-1;
  uint32_t nUnitId = (uint32_t)-1;
  char szName[128] = "";

  if (args.get(0).isString()) {
    const wchar_t* szText = JS_GetStringCharsZ(cx, args[0].toString());
    char* euc = UnicodeToAnsi(szText, CP_ACP);
    strcpy_s(szName, sizeof(szName), euc);
    delete[] euc;
  }

  if (args.get(0).isNumber() && !args[0].isNull()) JS_ValueToECMAUint32(cx, args[0], &nClassId);

  if (args.get(1).isNumber() && !args[1].isNull()) JS_ValueToECMAUint32(cx, args[1], &nMode);

  if (args.get(2).isNumber() && !args[2].isNull()) JS_ValueToECMAUint32(cx, args[2], &nUnitId);

  UnitAny* pItem = GetInvUnit(pUnit, szName, nClassId, nMode, nUnitId);

  if (!pItem) return JS_TRUE;

  invUnit* pmyItem = new invUnit;  // leaked?

  if (!pmyItem) return JS_TRUE;

  pmyItem->_dwPrivateType = PRIVATE_ITEM;
  pmyItem->dwClassId = nClassId;
  pmyItem->dwMode = nMode;
  pmyItem->dwType = pItem->dwType;
  pmyItem->dwUnitId = pItem->dwUnitId;
  pmyItem->dwOwnerId = pmyUnit->dwUnitId;
  pmyItem->dwOwnerType = pmyUnit->dwType;
  strcpy_s(pmyItem->szName, sizeof(pmyItem->szName), szName);

  JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyItem);

  if (!jsunit) return JS_TRUE;

  args.rval().setObjectOrNull(jsunit);
  return JS_TRUE;
}

JSAPI_FUNC(unit_move) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv().toObjectOrNull();
  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(self);

  if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

  UnitAny* pPlayer = D2CLIENT_GetPlayerUnit();

  if (!pPlayer || !pUnit) return JS_TRUE;

  int32_t x, y;

  if (pUnit == pPlayer) {
    if (args.length() < 2) return JS_TRUE;

    if (JS_ValueToInt32(cx, args[0], &x) == JS_FALSE) return JS_TRUE;
    if (JS_ValueToInt32(cx, args[1], &y) == JS_FALSE) return JS_TRUE;
  } else {
    x = D2CLIENT_GetUnitX(pUnit);
    y = D2CLIENT_GetUnitY(pUnit);
  }

  ClickMap(0, (WORD)x, (WORD)y, FALSE, NULL);
  Sleep(50);
  ClickMap(2, (WORD)x, (WORD)y, FALSE, NULL);
  //	D2CLIENT_Move((WORD)x, (WORD)y);
  return JS_TRUE;
}

JSAPI_FUNC(unit_getEnchant) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (!args.get(0).isInt32()) return JS_TRUE;

  auto self = args.thisv().toObjectOrNull();
  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(self);

  if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

  if (!pUnit || pUnit->dwType != UNIT_MONSTER) return JS_TRUE;

  int nEnchant = args[0].toInt32();
  args.rval().setInt32(0);

  for (int i = 0; i < 9; i++)
    if (pUnit->pMonsterData->anEnchants[i] == nEnchant) {
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
      args.rval().setBoolean(true);
      break;
    }

  return JS_TRUE;
}

JSAPI_FUNC(unit_getQuest) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (!args.get(0).isInt32() || !args.get(1).isInt32()) return JS_TRUE;

  int32_t nAct = args[0].toInt32();
  int32_t nQuest = args[1].toInt32();

  args.rval().setInt32(D2COMMON_GetQuestFlag(D2CLIENT_GetQuestInfo(), nAct, nQuest));
  return JS_TRUE;
}

JSAPI_FUNC(unit_getMinionCount) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (!args.get(0).isInt32()) return JS_TRUE;

  int32_t nType = args[0].toInt32();

  auto self = args.thisv().toObjectOrNull();
  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(self);

  if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

  if (!pUnit || (pUnit->dwType != UNIT_MONSTER && pUnit->dwType != UNIT_PLAYER)) return JS_TRUE;

  args.rval().setInt32(D2CLIENT_GetMinionCount(pUnit, nType));
  return JS_TRUE;
}

JSAPI_FUNC(me_getRepairCost) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  UnitAny* npc = D2CLIENT_GetCurrentInteractingNPC();
  int32_t nNpcClassId = (npc ? npc->dwTxtFileNo : 0x9A);

  if (args.get(0).isInt32()) nNpcClassId = args[0].toInt32();

  args.rval().setInt32(D2COMMON_GetRepairCost(NULL, D2CLIENT_GetPlayerUnit(), nNpcClassId, D2CLIENT_GetDifficulty(),
                                              *p_D2CLIENT_ItemPriceList, 0));
  return JS_TRUE;
}
