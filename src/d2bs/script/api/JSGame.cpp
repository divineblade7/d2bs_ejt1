#include "d2bs/script/api/JSGame.h"

#include "d2bs/core/Core.h"
#include "d2bs/core/Game.h"
#include "d2bs/core/MPQStats.h"
#include "d2bs/core/Unit.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Skills.h"
#include "d2bs/script/api/JSArea.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSRoom.h"
#include "d2bs/utils/CriticalSections.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/utils/MapHeader.h"
#include "d2bs/utils/TimedAlloc.h"

#include <cassert>
#include <cmath>

JSAPI_FUNC(my_copyUnit) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setUndefined();

  if (args.length() >= 1 && args[0].isObject() && !args[0].isNull()) {
    Private* myPrivate = (Private*)JS_GetPrivate(args[0].toObjectOrNull());

    if (!myPrivate) {
      return JS_TRUE;
    }

    if (myPrivate->dwPrivateType == PRIVATE_UNIT) {
      myUnit* lpOldUnit = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());
      myUnit* lpUnit = new myUnit;

      if (lpUnit) {
        memcpy(lpUnit, lpOldUnit, sizeof(myUnit));
        JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, lpUnit);
        if (!jsunit) {
          delete lpUnit;
          lpUnit = NULL;
          THROW_ERROR(cx, "Couldn't copy unit");
        }

        JS_SetPrivate(jsunit, lpUnit);
        args.rval().setObjectOrNull(jsunit);
      }
    } else if (myPrivate->dwPrivateType == PRIVATE_ITEM) {
      invUnit* lpOldUnit = (invUnit*)JS_GetPrivate(args[0].toObjectOrNull());
      invUnit* lpUnit = new invUnit;

      if (lpUnit) {
        memcpy(lpUnit, lpOldUnit, sizeof(invUnit));
        JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, lpUnit);
        if (!jsunit) {
          delete lpUnit;
          lpUnit = NULL;
          THROW_ERROR(cx, "Couldn't copy unit");
        }

        JS_SetPrivate(jsunit, lpUnit);
        args.rval().setObjectOrNull(jsunit);
      }
    }
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_clickMap) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  uint16 nClickType = NULL, nShift = NULL, nX = NULL, nY = NULL;

  args.rval().setBoolean(false);

  if (args.length() < 3) {
    return JS_TRUE;
  }

  {
    JSAutoRequest r(cx);
    if (args.get(0).isInt32()) nClickType = static_cast<uint16_t>(args[0].toInt32());
    if (args.get(1).isInt32() || args[1].isBoolean()) JS_ValueToUint16(cx, args[1], &nShift);
    if (args.get(2).isInt32()) nX = static_cast<uint16_t>(args[2].toInt32());
    if (args.get(3).isInt32()) nY = static_cast<uint16_t>(args[3].toInt32());
  }

  if (args.length() == 3 && args[0].isInt32() && (args[1].isInt32() || args[1].isBoolean()) && args[2].isObject() &&
      !args[2].isNull()) {
    myUnit* mypUnit = (myUnit*)JS_GetPrivate(args[2].toObjectOrNull());

    if (!mypUnit || (mypUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) {
      return JS_TRUE;
    }

    UnitAny* pUnit = D2CLIENT_FindUnit(mypUnit->dwUnitId, mypUnit->dwType);

    if (!pUnit) {
      return JS_TRUE;
    }

    Vars.dwSelectedUnitId = NULL;
    Vars.dwSelectedUnitType = NULL;
    args.rval().setBoolean(ClickMap(nClickType, nX, nY, nShift, pUnit));
  } else if (args.length() > 3 && args[0].isInt32() && (args[1].isInt32() || args[1].isBoolean()) &&
             args[2].isInt32() && args[3].isInt32()) {
    args.rval().setBoolean(ClickMap(nClickType, nX, nY, nShift, nullptr));
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_acceptTrade) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (args.length() > 0 && args[0].isInt32()) {
    switch (args[0].toInt32()) {
      case 1:  // Called with a '1' it will return if we already accepted it or not
        args.rval().setBoolean(*p_D2CLIENT_bTradeAccepted);
        break;
      case 2:  // Called with a '2' it will return the trade flag
        args.rval().setInt32(*p_D2CLIENT_RecentTradeId);
        break;
      case 3:  // Called with a '3' it will return if the 'check' is red or not
        args.rval().setBoolean(*p_D2CLIENT_bTradeBlock);
        break;
      default:
        THROW_ERROR(cx, "acceptTrade called with invalid parameter");
    }

    return JS_TRUE;
  }

  CriticalRoom cRoom;

  if ((*p_D2CLIENT_RecentTradeId) == 3 || (*p_D2CLIENT_RecentTradeId) == 5 || (*p_D2CLIENT_RecentTradeId) == 7) {
    if ((*p_D2CLIENT_bTradeBlock)) {
      // Don't operate if we can't trade anyway ...
      args.rval().setBoolean(false);
    } else if ((*p_D2CLIENT_bTradeAccepted)) {
      (*p_D2CLIENT_bTradeAccepted) = FALSE;
      D2CLIENT_CancelTrade();
      args.rval().setBoolean(true);
    } else {
      (*p_D2CLIENT_bTradeAccepted) = TRUE;
      D2CLIENT_AcceptTrade();
      args.rval().setBoolean(true);
    }
    return JS_TRUE;
  }
  THROW_ERROR(cx, "Invalid parameter passed to acceptTrade!");
}

JSAPI_FUNC(my_tradeOk) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  CriticalRoom cRoom;
  TransactionDialogsInfo_t* pTdi = *p_D2CLIENT_pTransactionDialogsInfo;
  unsigned int i;

  if (pTdi != NULL) {
    for (i = 0; i < pTdi->numLines; ++i) {
      // Not sure if *p_D2CLIENT_TransactionDialogs == 1 necessary if it's in
      // the dialog list, but if it's not 1, a crash is guaranteed. (CrazyCasta)
      if (pTdi->dialogLines[i].handler == D2CLIENT_TradeOK && *p_D2CLIENT_TransactionDialogs == 1) {
        D2CLIENT_TradeOK();
        args.rval().setUndefined();
        return JS_TRUE;
      }
    }
  }
  THROW_ERROR(cx, "Not in proper state to click ok to trade.");
}

JSAPI_FUNC(my_getDialogLines) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setNull();

  TransactionDialogsInfo_t* pTdi = *p_D2CLIENT_pTransactionDialogsInfo;
  unsigned int i;
  JSObject* line;
  jsval js_text, js_selectable, js_line, js_addr;
  JSFunction* jsf_handler;

  CriticalRoom cRoom;

  if (pTdi != NULL) {
    JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));
    for (i = 0; i < pTdi->numLines; ++i) {
      js_text = JS::StringValue(JS_NewUCStringCopyZ(cx, pTdi->dialogLines[i].text));
      js_selectable = JS::BooleanValue(pTdi->dialogLines[i].bMaybeSelectable);

      line = BuildObject(cx, &dialogLine_class, 0, 0, 0, 0);
      JS_SetProperty(cx, line, "text", &js_text);
      JS_SetProperty(cx, line, "selectable", &js_selectable);

      jsf_handler = JS_DefineFunction(cx, line, "handler", my_clickDialog, 0, JSPROP_ENUMERATE);
      js_addr = JS::ObjectOrNullValue(JS_NewObject(cx, 0, 0, pReturnArray));
      // line -> JS_GetFunctionObject(jsf_handler) in 1.8.5

      // handler() or clickDialog() needs the private handler val stored somewhere
      JS_SetReservedSlot(line, 0, PRIVATE_TO_JSVAL(&pTdi->dialogLines[i]));
      // these privates dident work dident have private slots
      // JS_SetPrivate(JS_GetFunctionObject(jsf_handler), &pTdi->dialogLines[i]);
      // JS_SetPrivate(line,  &pTdi->dialogLines[i]);
      js_line = JS::ObjectOrNullValue(line);
      JS_SetElement(cx, pReturnArray, i, &js_line);
    }

    args.rval().setObjectOrNull(pReturnArray);
  }

  return JS_TRUE;
}
JSAPI_FUNC(my_clickDialog) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  TransactionDialogsLine_t* tdl;

  tdl = (TransactionDialogsLine_t*)JSVAL_TO_PRIVATE(JS_GetReservedSlot(JSVAL_TO_OBJECT(JS_THIS(cx, vp)), 0));

  if (tdl != NULL && tdl->bMaybeSelectable)
    tdl->handler();
  else
    THROW_ERROR(cx, "That dialog is not currently clickable.");

  JS_SET_RVAL(cx, vp, JSVAL_TRUE);
  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_getPath) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (args.length() < 5) THROW_ERROR(cx, "Not enough parameters were passed to getPath!");

  uint lvl = 0, x = 0, y = 0, dx = 0, dy = 0, reductionType = 0, radius = 20;

  {
    JSAutoRequest r(cx);
    if (!JS_ConvertArguments(cx, argc, args.array(), "uuuuu/uu", &lvl, &x, &y, &dx, &dy, &reductionType, &radius)) {
      return JS_FALSE;
    }
  }

  if (reductionType == 3 &&
      !(JSVAL_IS_FUNCTION(cx, args[7]) && JSVAL_IS_FUNCTION(cx, args[8]) && JSVAL_IS_FUNCTION(cx, args[9]))) {
    THROW_ERROR(cx, "Invalid function values for reduction type");
  }
  if (lvl == 0) {
    THROW_ERROR(cx, "Invalid level passed to getPath");
  }
  Level* level = GetLevel(lvl);

  if (!level) return JS_FALSE;

  ActMap* map = ActMap::GetMap(level);

  Point start(x, y), end(dx, dy);

  std::unique_ptr<PathReducer> reducer;
  switch (reductionType) {
    case 0:
      reducer = std::make_unique<WalkPathReducer>(map, DiagonalShortcut, radius);
      break;
    case 1:
      reducer = std::make_unique<TeleportPathReducer>(map, DiagonalShortcut, radius);
      break;
    case 2:
      reducer = std::make_unique<NoPathReducer>(map);
      break;
    case 3:
      reducer = std::make_unique<JSPathReducer>(map, cx, args.thisv().toObjectOrNull(), args[7], args[8], args[9]);
      break;
    default:
      THROW_ERROR(cx, "Invalid path reducer value!");
      break;
  }

  PointList list;
#if defined(_TIME)
  AStarPath<TimedAlloc<Node, std::allocator<Node>>> path(map, reducer.get());
#else
  AStarPath<> path(map, reducer.get());
#endif

  path.GetPath(start, end, list, true);
  map->CleanUp();
#if defined(_TIME)
  char p[510];
  sprintf_s(p, 510, "%s\\stats.txt", Vars.szPath);
  FILE* f;
  fopen_s(&f, p, "a+");
  path.GetAllocator().DumpStats(f);
  fclose(f);
#endif

  int count = list.size();

  jsval* vec = new jsval[count];
  for (int i = 0; i < count; i++) {
    jsval jx = JS::Int32Value(list[i].first), jy = JS::Int32Value(list[i].second);

    JSObject* point = BuildObject(cx);
    JS_SetProperty(cx, point, "x", &jx);
    JS_SetProperty(cx, point, "y", &jy);

    vec[i] = JS::ObjectOrNullValue(point);
  }

  JSObject* arr = JS_NewArrayObject(cx, count, vec);
  args.rval().setObjectOrNull(arr);

  reducer.reset();
  map->CleanUp();
  return JS_TRUE;
}

JSAPI_FUNC(my_getCollision) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) {
    THROW_WARNING(cx, vp, "Game not ready");
  }

  uint32 nLevelId, nX, nY;
  {
    JSAutoRequest r(cx);
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "uuu", &nLevelId, &nX, &nY)) {
      return JS_FALSE;
    }
  }

  Point point(nX, nY);
  Level* level = GetLevel(nLevelId);
  if (!level) {
    THROW_ERROR(cx, "Level Not loaded");
  }

  ActMap* map = ActMap::GetMap(level);

  jsval rval;
  {
    JSAutoRequest r(cx);
    rval = JS_NumberValue(map->GetMapData(point, true));
  }
  args.rval().set(rval);
  map->CleanUp();

  return JS_TRUE;
}

JSAPI_FUNC(my_clickItem) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  typedef void __fastcall clickequip(UnitAny * pPlayer, Inventory * pIventory, int loc);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  CriticalRoom cRoom;

  if (*p_D2CLIENT_TransactionDialog != 0 || *p_D2CLIENT_TransactionDialogs != 0 ||
      *p_D2CLIENT_TransactionDialogs_2 != 0) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  myUnit* pmyUnit = NULL;
  UnitAny* pUnit = NULL;

  // int ScreenSize = D2GFX_GetScreenSize();

  POINT Belt[] = {
      {0, 0},  // 0
      {1, 0},  // 1
      {2, 0},  // 2
      {3, 0},  // 3

      {0, 1},  // 4
      {1, 1},  // 5
      {2, 1},  // 6
      {3, 1},  // 7

      {0, 2},  // 8
      {1, 2},  // 9
      {2, 2},  // 10
      {3, 2},  // 11

      {0, 3},  // 12
      {1, 3},  // 13
      {2, 3},  // 14
      {3, 3},  // 15
  };

  *p_D2CLIENT_CursorHoverX = 0xFFFFFFFF;
  *p_D2CLIENT_CursorHoverY = 0xFFFFFFFF;

  args.rval().setUndefined();
  if (args.length() == 1 && args[0].isObjectOrNull()) {
    pmyUnit = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());

    if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) {
      return JS_TRUE;
    }

    pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

    if (!pUnit) {
      return JS_TRUE;
    }

    clickequip* click = (clickequip*)*(DWORD*)(D2CLIENT_BodyClickTable + (4 * pUnit->pItemData->BodyLocation));

    if (!click) {
      return JS_TRUE;
    }

    click(D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, pUnit->pItemData->BodyLocation);
    return JS_TRUE;
  } else if (args.length() == 2 && args[0].isInt32() && args[1].isInt32()) {
    jsint nClickType = args[0].toInt32();
    jsint nBodyLoc = args[1].toInt32();

    if (nClickType == NULL) {
      clickequip* click = (clickequip*)*(DWORD*)(D2CLIENT_BodyClickTable + (4 * nBodyLoc));

      if (!click) {
        return JS_TRUE;
      }

      click(D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, nBodyLoc);

    }
    // Click Merc Gear
    else if (nClickType == 4) {
      if (nBodyLoc == 1 || nBodyLoc == 3 || nBodyLoc == 4) {
        UnitAny* pMerc = GetMercUnit(D2CLIENT_GetPlayerUnit());

        if (pMerc) {
          D2CLIENT_MercItemAction(0x61, nBodyLoc);
          args.rval().setBoolean(true);
        }
      }
    }
    return JS_TRUE;
  } else if (args.length() == 2 && args[0].isInt32() && args[1].isObject()) {
    pmyUnit = (myUnit*)JS_GetPrivate(args[1].toObjectOrNull());

    if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) {
      return JS_TRUE;
    }

    pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

    jsint nClickType = args[0].toInt32();

    if (!pUnit || !(pUnit->dwType == UNIT_ITEM) || !pUnit->pItemData) {
      THROW_ERROR(cx, "Object is not an item!");
    }

    int InventoryLocation = GetItemLocation(pUnit);
    int ClickLocation = LOCATION_NULL;

    int x = pUnit->pItemPath->dwPosX;
    int y = pUnit->pItemPath->dwPosY;

    *p_D2CLIENT_CursorHoverX = x;
    *p_D2CLIENT_CursorHoverY = y;

    InventoryLayout* pLayout = NULL;

    if (nClickType == 4) {
      UnitAny* pMerc = GetMercUnit(D2CLIENT_GetPlayerUnit());

      if (pMerc)
        if (pUnit->pItemData && pUnit->pItemData->pOwner)
          if (pUnit->pItemData->pOwner->dwUnitId == pMerc->dwUnitId) {
            args.rval().setBoolean(true);
            D2CLIENT_MercItemAction(0x61, pUnit->pItemData->BodyLocation);
          }
      return JS_TRUE;
    } else if (InventoryLocation == LOCATION_INVENTORY || InventoryLocation == LOCATION_STASH ||
               InventoryLocation == LOCATION_CUBE) {
      switch (InventoryLocation) {
        case LOCATION_INVENTORY:
          pLayout = (InventoryLayout*)p_D2CLIENT_InventoryLayout;
          ClickLocation = CLICKTARGET_INVENTORY;
          break;
        case LOCATION_STASH:
          pLayout = (InventoryLayout*)p_D2CLIENT_StashLayout;
          ClickLocation = CLICKTARGET_STASH;
          break;
        case LOCATION_CUBE:
          pLayout = (InventoryLayout*)p_D2CLIENT_CubeLayout;
          ClickLocation = CLICKTARGET_CUBE;
          break;
      }

      x = pLayout->Left + x * pLayout->SlotPixelWidth + 10;
      y = pLayout->Top + y * pLayout->SlotPixelHeight + 10;

      if (nClickType == NULL)
        D2CLIENT_LeftClickItem(ClickLocation, D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, x, y,
                               nClickType, pLayout);
      // D2CLIENT_LeftClickItem(D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, x, y, nClickType,
      // pLayout, pUnit->pItemData->ItemLocation);
      else
        D2CLIENT_RightClickItem(x, y, ClickLocation, D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory);
      // D2CLIENT_RightClickItem(x,y, pUnit->pItemData->ItemLocation, D2CLIENT_GetPlayerUnit(),
      // D2CLIENT_GetPlayerUnit()->pInventory);

    } else if (InventoryLocation == LOCATION_BELT) {
      int i = x;

      if (i < 0 || i > 0x0F) {
        return JS_TRUE;
      }

      if (D2GFX_GetScreenSize() == 2) {
        x = 440 + (Belt[i].x * 29);
        y = 580 - (Belt[i].y * 29);
      } else {
        x = 360 + (Belt[i].x * 29);
        y = 460 - (Belt[i].y * 29);
      }
      if (nClickType == NULL)
        D2CLIENT_ClickBelt(x, y, D2CLIENT_GetPlayerUnit()->pInventory);
      else
        D2CLIENT_ClickBeltRight(D2CLIENT_GetPlayerUnit()->pInventory, D2CLIENT_GetPlayerUnit(),
                                nClickType == 1 ? FALSE : TRUE, i);
    } else if (D2CLIENT_GetCursorItem() == pUnit) {
      if (nClickType < 1 || nClickType > 12) {
        return JS_TRUE;
      }

      clickequip* click = (clickequip*)*(DWORD*)(D2CLIENT_BodyClickTable + (4 * nClickType));

      if (!click) {
        return JS_TRUE;
      }

      click(D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, nClickType);
    }
  } else if (args.length() == 4) {
    if (args[0].isInt32() && args[1].isInt32() && args[2].isInt32() && args[3].isInt32()) {
      jsint nButton = args[0].toInt32();
      jsint nX = args[1].toInt32();
      jsint nY = args[2].toInt32();
      jsint nLoc = args[3].toInt32();

      int clickTarget = LOCATION_NULL;
      InventoryLayout* pLayout = NULL;

      *p_D2CLIENT_CursorHoverX = nX;
      *p_D2CLIENT_CursorHoverY = nY;

      // Fixing the x/y click spot for items taking more than one inventory square- so Diablo can handle it!
      if (nLoc != LOCATION_BELT) {
        UnitAny* pItem = D2CLIENT_GetCursorItem();
        if (pItem) {
          ItemTxt* pTxt = D2COMMON_GetItemText(pItem->dwTxtFileNo);
          if (pTxt) {
            if (pTxt->ySize > 1) nY += 1;

            if (pTxt->xSize > 1) nX += 1;
          }
        }
      }
      // nLoc is click target locations=: LOCATION_INVENTORY=inventory, LOCATION_TRADE=player trade, LOCATION_CUBE=cube,
      // LOCATION_STASH=stash, LOCATION_BELT=belt
      if (nLoc == LOCATION_INVENTORY || nLoc == LOCATION_TRADE || nLoc == LOCATION_CUBE || nLoc == LOCATION_STASH) {
        switch (nLoc) {
          case LOCATION_INVENTORY:
            pLayout = (InventoryLayout*)p_D2CLIENT_InventoryLayout;
            clickTarget = CLICKTARGET_INVENTORY;
            break;
          case LOCATION_TRADE:
            pLayout = (InventoryLayout*)p_D2CLIENT_TradeLayout;
            clickTarget = CLICKTARGET_TRADE;
            break;
          case LOCATION_CUBE:
            pLayout = (InventoryLayout*)p_D2CLIENT_CubeLayout;
            clickTarget = CLICKTARGET_CUBE;
            break;
          case LOCATION_STASH:
            pLayout = (InventoryLayout*)p_D2CLIENT_StashLayout;
            clickTarget = CLICKTARGET_STASH;
            break;
        }

        int x = pLayout->Left + nX * pLayout->SlotPixelWidth + 10;
        int y = pLayout->Top + nY * pLayout->SlotPixelHeight + 10;

        if (nButton == 0)  // Left Click
          D2CLIENT_LeftClickItem(clickTarget, D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, x, y, 1,
                                 pLayout);
        else if (nButton == 1)  // Right Click
          D2CLIENT_RightClickItem(x, y, clickTarget, D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory);
        else if (nButton == 2)  // Shift Left Click
          D2CLIENT_LeftClickItem(clickTarget, D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, x, y, 5,
                                 pLayout);

        args.rval().setBoolean(true);
        return JS_TRUE;
      } else if (nLoc == LOCATION_BELT)  // Belt
      {
        int z = -1;

        for (UINT i = 0; i < ArraySize(Belt); i++) {
          if (Belt[i].x == nX && Belt[i].y == nY) {
            z = (int)i;
            break;
          }
        }

        if (z == -1) {
          return JS_TRUE;
        }

        int x = NULL;
        int y = NULL;

        if (D2GFX_GetScreenSize() == 2) {
          x = 440 + (Belt[z].x * 29);
          y = 580 - (Belt[z].y * 29);
        } else {
          x = 360 + (Belt[z].x * 29);
          y = 460 - (Belt[z].y * 29);
        }

        if (nButton == 0)
          D2CLIENT_ClickBelt(x, y, D2CLIENT_GetPlayerUnit()->pInventory);
        else if (nButton == 1)
          D2CLIENT_ClickBeltRight(D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, FALSE, z);
        else if (nButton == 2)
          D2CLIENT_ClickBeltRight(D2CLIENT_GetPlayerUnit(), D2CLIENT_GetPlayerUnit()->pInventory, TRUE, z);

        args.rval().setBoolean(true);
        return JS_TRUE;
      }
    }
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_getLocaleString) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 1 || !args[0].isInt32()) return JS_TRUE;

  uint16 localeId;
  JSAutoRequest r(cx);
  JS_ValueToUint16(cx, args[0], &localeId);
  wchar_t* wString = D2LANG_GetLocaleText(localeId);
  args.rval().setString(JS_NewUCStringCopyZ(cx, wString));
  return JS_TRUE;
}

JSAPI_FUNC(my_rand) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 2 || !args[0].isInt32() || !args[1].isInt32()) {
    args.rval().setInt32(0);
    return JS_TRUE;
  }

  // only seed the rng once
  static bool seeded = false;
  if (!seeded) {
    srand(GetTickCount());
    seeded = true;
  }

  long long seed = 0;
  if (ClientState() == ClientStateInGame)
    seed = D2GAME_Rand(D2CLIENT_GetPlayerUnit()->dwSeed);
  else
    seed = rand();

  jsint high;
  jsint low;
  {
    JSAutoRequest r(cx);
    if (JS_ConvertArguments(cx, 2, args.array(), "ii", &low, &high) == JS_FALSE) {
      THROW_ERROR(cx, "Could not convert Rand aruments");
    }
  }

  if (high > low + 1) {
    int i = (seed % (high - low + 1)) + low;
    args.rval().setInt32(i);
  } else {
    args.rval().setInt32(high);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getDistance) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  // TODO: Add the type of distance to the api design
  jsint nX1 = NULL;
  jsint nX2 = NULL;
  jsint nY1 = NULL;
  jsint nY2 = NULL;

  JSAutoRequest r(cx);
  if (args.length() == 1 && args[0].isObject()) {
    jsval x1, y1;
    if (JS_GetProperty(cx, args[0].toObjectOrNull(), "x", &x1) &&
        JS_GetProperty(cx, args[0].toObjectOrNull(), "y", &y1)) {
      nX1 = D2CLIENT_GetUnitX(D2CLIENT_GetPlayerUnit());
      nY1 = D2CLIENT_GetUnitY(D2CLIENT_GetPlayerUnit());
      JS_ValueToECMAInt32(cx, x1, &nX2);
      JS_ValueToECMAInt32(cx, y1, &nY2);
    }
  } else if (args.length() == 2) {
    if (args[0].isInt32() && args[1].isInt32()) {
      nX1 = D2CLIENT_GetUnitX(D2CLIENT_GetPlayerUnit());
      nY1 = D2CLIENT_GetUnitY(D2CLIENT_GetPlayerUnit());
      JS_ValueToECMAInt32(cx, args[0], &nX2);
      JS_ValueToECMAInt32(cx, args[1], &nY2);
    } else if (args[0].isObject() && args[1].isObject()) {
      jsval x, y, x2, y2;
      if (JS_GetProperty(cx, args[0].toObjectOrNull(), "x", &x) &&
          JS_GetProperty(cx, args[0].toObjectOrNull(), "y", &y) &&
          JS_GetProperty(cx, args[1].toObjectOrNull(), "x", &x2) &&
          JS_GetProperty(cx, args[1].toObjectOrNull(), "y", &y2)) {
        JS_ValueToECMAInt32(cx, x, &nX1);
        JS_ValueToECMAInt32(cx, y, &nY1);
        JS_ValueToECMAInt32(cx, x2, &nX2);
        JS_ValueToECMAInt32(cx, y2, &nY2);
      }
    }
  } else if (args.length() == 3) {
    if (args[0].isObject() && args[1].isInt32() && args[2].isInt32()) {
      myUnit* pUnit1 = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());

      if (!pUnit1 || (pUnit1->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

      UnitAny* pUnitA = D2CLIENT_FindUnit(pUnit1->dwUnitId, pUnit1->dwType);

      if (!pUnitA) return JS_TRUE;

      nX1 = D2CLIENT_GetUnitX(pUnitA);
      nY1 = D2CLIENT_GetUnitY(pUnitA);
      JS_ValueToECMAInt32(cx, args[1], &nX2);
      JS_ValueToECMAInt32(cx, args[2], &nY2);
    } else if (args[0].isInt32() && args[1].isInt32() && args[2].isObject()) {
      myUnit* pUnit1 = (myUnit*)JS_GetPrivate(args[2].toObjectOrNull());

      if (!pUnit1 || (pUnit1->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

      UnitAny* pUnitA = D2CLIENT_FindUnit(pUnit1->dwUnitId, pUnit1->dwType);

      if (!pUnitA) return JS_TRUE;

      nX1 = D2CLIENT_GetUnitX(pUnitA);
      nY1 = D2CLIENT_GetUnitY(pUnitA);
      JS_ValueToECMAInt32(cx, args[0], &nX2);
      JS_ValueToECMAInt32(cx, args[1], &nY2);
    }
  } else if (args.length() == 4) {
    if (args[0].isInt32() && args[1].isInt32() && args[2].isInt32() && args[3].isInt32()) {
      JS_ValueToECMAInt32(cx, args[0], &nX1);
      JS_ValueToECMAInt32(cx, args[1], &nY1);
      JS_ValueToECMAInt32(cx, args[2], &nX2);
      JS_ValueToECMAInt32(cx, args[3], &nY2);
    }
  }

  jsdouble jsdist = (jsdouble)abs(GetDistance(nX1, nY1, nX2, nY2));
  jsval rval;
  rval = JS_NumberValue(jsdist);
  args.rval().set(rval);
  return JS_TRUE;
}

JSAPI_FUNC(my_gold) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  jsint nGold = NULL;
  jsint nMode = 1;

  if (args.length() > 0 && args[0].isInt32()) nGold = args[0].toInt32();

  if (args.length() > 1 && args[1].isInt32()) nMode = args[1].toInt32();

  SendGold(nGold, nMode);
  return JS_TRUE;
}

JSAPI_FUNC(my_checkCollision) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (args.length() == 3 && args[0].isObject() && args[1].isObject() && args[2].isInt32()) {
    myUnit* pUnitA = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());
    myUnit* pUnitB = (myUnit*)JS_GetPrivate(args[1].toObjectOrNull());
    jsint nBitMask = args[2].toInt32();

    if (!pUnitA || (pUnitA->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT || !pUnitB ||
        (pUnitB->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT)
      return JS_TRUE;

    UnitAny* pUnit1 = D2CLIENT_FindUnit(pUnitA->dwUnitId, pUnitA->dwType);
    UnitAny* pUnit2 = D2CLIENT_FindUnit(pUnitB->dwUnitId, pUnitB->dwType);

    if (!pUnit1 || !pUnit2) return JS_TRUE;

    args.rval().setInt32(D2COMMON_CheckUnitCollision(pUnit1, pUnit2, static_cast<WORD>(nBitMask)));
    return JS_TRUE;
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getCursorType) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  jsint nType = NULL;

  if (args.length() > 0) nType = args[0].toInt32();

  args.rval().setInt32(nType == 1 ? *p_D2CLIENT_ShopCursorType : *p_D2CLIENT_RegularCursorType);
  return JS_TRUE;
}

JSAPI_FUNC(my_getSkillByName) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 1 || !args[0].isString()) return JS_TRUE;

  char* lpszText = JS_EncodeStringToUTF8(cx, JS_ValueToString(cx, args[0]));
  if (!lpszText || lpszText[0]) THROW_ERROR(cx, "Could not convert string");

  for (int i = 0; i < ArraySize(Game_Skills); i++) {
    if (!_strcmpi(Game_Skills[i].name, lpszText)) {
      args.rval().setInt32(Game_Skills[i].skillID);
      break;
    }
  }

  JS_free(cx, lpszText);
  return JS_TRUE;
}

JSAPI_FUNC(my_getSkillById) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 1 || !args[0].isInt32()) return JS_TRUE;

  jsint nId = args[0].toInt32();
  JSAutoRequest r(cx);
  args.rval().setString(JS_NewStringCopyZ(cx, "Unknown"));
  int row = 0;
  if (FillBaseStat("skills", nId, "skilldesc", &row, sizeof(int))) {
    if (FillBaseStat("skilldesc", row, "str name", &row, sizeof(int))) {
      wchar_t* szName = D2LANG_GetLocaleText((WORD)row);
      args.rval().setString(JS_NewUCStringCopyZ(cx, szName));
    }
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getTextSize) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 2 || !args[0].isString() || !args[1].isInt32()) {
    JS_SET_RVAL(cx, vp, JSVAL_FALSE);
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  const wchar_t* pString = JS_GetStringCharsZ(cx, args[0].toString());
  if (!pString) THROW_ERROR(cx, "Could not convert string");

  POINT r = CalculateTextLen(pString, JSVAL_TO_INT(JS_ARGV(cx, vp)[1]));
  jsval x = JS::Int32Value(r.x);
  jsval y = JS::Int32Value(r.y);

  JS::RootedObject pObj(cx);
  if (args.length() > 2 && args[2].isBoolean() && args[2].toBoolean() == TRUE) {
    // return an object with a height/width rather than an array
    pObj = BuildObject(cx, NULL);
    if (!pObj) THROW_ERROR(cx, "Could not build object");
    if (JS_SetProperty(cx, pObj, "width", &x) == JS_FALSE) THROW_ERROR(cx, "Could not set width property");
    if (JS_SetProperty(cx, pObj, "height", &y) == JS_FALSE) THROW_ERROR(cx, "Could not set height property");
  } else {
    JSAutoRequest req(cx);
    pObj = JS_NewArrayObject(cx, NULL, NULL);
    JS_SetElement(cx, pObj, 0, &x);
    JS_SetElement(cx, pObj, 1, &y);
  }

  args.rval().setObjectOrNull(pObj);
  return JS_TRUE;
}

JSAPI_FUNC(my_getTradeInfo) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // set default return value to false
  args.rval().setBoolean(false);

  if (args.length() < 1) return JS_TRUE;

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (!args[0].isInt32()) return JS_TRUE;

  jsint nMode = args[0].toInt32();

  switch (nMode) {
    case 0:
      args.rval().setInt32(*p_D2CLIENT_RecentTradeId);
      break;
    case 1: {
      // FIXME
      // char* tmp = UnicodeToAnsi((wchar_t*)(*p_D2CLIENT_RecentTradeName));
      //*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp));
      // delete[] tmp;
      // Temporary return value to keep it kosher
      args.rval().setNull();
    } break;
    case 2:
      args.rval().setInt32(*p_D2CLIENT_RecentTradeId);
      break;
    default:
      args.rval().setBoolean(false);
      break;
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getUIFlag) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 1 || !args[0].isInt32()) {
    args.rval().setUndefined();
    return JS_TRUE;
  }

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  jsint nUIId = args[0].toInt32();

  args.rval().setBoolean(D2CLIENT_GetUIState(nUIId));
  return JS_TRUE;
}

JSAPI_FUNC(my_getWaypoint) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 1 || !args[0].isInt32()) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  jsint nWaypointId = args[0].toInt32();

  if (nWaypointId > 40) nWaypointId = NULL;

  args.rval().setBoolean(!!D2COMMON_CheckWaypoint((*p_D2CLIENT_WaypointTable), nWaypointId));
  return JS_TRUE;
}

JSAPI_FUNC(my_quitGame) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setBoolean(false);
  if (ClientState() != ClientStateMenu) D2CLIENT_ExitGame();

  // give the core a chance to shut down
  // sEngine->shutdown(true);
  TerminateProcess(GetCurrentProcess(), 0);

  return JS_TRUE;
}

JSAPI_FUNC(my_quit) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setBoolean(false);
  Vars.bQuitting = true;
  if (ClientState() != ClientStateMenu) D2CLIENT_ExitGame();

  return JS_TRUE;
}

JSAPI_FUNC(my_playSound) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // I need to take a closer look at the D2CLIENT_PlaySound function
  if (args.length() < 1 || !args[0].isInt32()) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  // jsint nSoundId = JSVAL_TO_INT(JS_ARGV(cx, vp)[0]);
  //  D2CLIENT_PlaySound(nSoundId);

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_say) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setBoolean(false);

  for (uint i = 0; i < args.length(); i++) {
    if (!args[i].isNull()) {
      JSAutoRequest r(cx);
      if (!JS_ConvertValue(cx, args[i], JSTYPE_STRING, &args[i])) {
        JS_ReportError(cx, "Converting to string failed");
        return JS_FALSE;
      }

      const wchar_t* Text = JS_GetStringCharsZ(cx, args[i].toString());
      if (Text == NULL) {
        JS_ReportError(cx, "Could not get string for value");
        return JS_FALSE;
      }
      if (Text) Say(L"%s", Text);
    }
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_clickParty) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setBoolean(false);

  if (args.length() < 2 || !args[0].isObject() || !args[1].isInt32()) return JS_TRUE;

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  UnitAny* myUnit = D2CLIENT_GetPlayerUnit();
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(args[0].toObjectOrNull());
  RosterUnit* mypUnit = *p_D2CLIENT_PlayerUnitList;

  if (!pUnit || !mypUnit) return JS_TRUE;

  BOOL bFound = FALSE;

  for (RosterUnit* pScan = mypUnit; pScan; pScan = pScan->pNext)
    if (pScan->dwUnitId == pUnit->dwUnitId) bFound = TRUE;

  if (!bFound) return JS_TRUE;

  jsint nMode = args[1].toInt32();

  BnetData* pData = (*p_D2LAUNCH_BnData);

  // Attempt to loot player, check first if it's hardcore
  if (nMode == 0 && pData && !(pData->nCharFlags & PLAYER_TYPE_HARDCORE)) return JS_TRUE;

  // Attempt to party a player who is already in party ^_~
  if (nMode == 2 && pUnit->wPartyId != 0xFFFF && mypUnit->wPartyId == pUnit->wPartyId && pUnit->wPartyId != 0xFFFF)
    return JS_TRUE;

  // don't leave a party if we are in none!
  if (nMode == 3 && pUnit->wPartyId == 0xFFFF)
    return JS_TRUE;
  else if (nMode == 3 && pUnit->wPartyId != 0xFFFF) {
    args.rval().setBoolean(true);
    D2CLIENT_LeaveParty();
    return JS_TRUE;
  }

  if (nMode < 0 || nMode > 5) return JS_TRUE;

  // Trying to click self
  if (pUnit->dwUnitId == myUnit->dwUnitId) return JS_TRUE;

  if (nMode == 0)
    D2CLIENT_HostilePartyUnit(pUnit, 2);
  else if (nMode == 1)
    D2CLIENT_HostilePartyUnit(pUnit, 1);
  else if (nMode == 4)
    D2CLIENT_HostilePartyUnit(pUnit, 3);
  else if (nMode == 5)
    D2CLIENT_HostilePartyUnit(pUnit, 4);
  else
    D2CLIENT_ClickParty(pUnit, nMode);

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_useStatPoint) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  WORD stat = 0;
  int32 count = 1;
  JSAutoRequest r(cx);
  if (!JS_ConvertArguments(cx, argc, args.array(), "c/u", &stat, &count)) {
    return JS_FALSE;
  }
  UseStatPoint(stat, count);

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_useSkillPoint) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  WORD skill = 0;
  int32 count = 1;
  JSAutoRequest r(cx);
  if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "c/u", &skill, &count)) {
    return JS_FALSE;
  }
  UseSkillPoint(skill, count);

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_getBaseStat) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() > 2) {
    char *szStatName = NULL, *szTableName = NULL;
    jsint nBaseStat = 0;
    jsint nClassId = 0;
    jsint nStat = -1;
    JSAutoRequest r(cx);
    if (args[0].isString()) {
      szTableName = JS_EncodeStringToUTF8(cx, args[0].toString());
      if (!szTableName) {
        THROW_ERROR(cx, "Invalid table value");
      }
    } else if (args[0].isNumber())
      JS_ValueToECMAInt32(cx, args[0], &nBaseStat);
    else {
      THROW_ERROR(cx, "Invalid table value");
    }

    JS_ValueToECMAInt32(cx, args[1], &nClassId);

    if (args[2].isString()) {
      szStatName = JS_EncodeStringToUTF8(cx, args[2].toString());
      if (!szStatName) {
        JS_free(cx, szTableName);
        THROW_ERROR(cx, "Invalid column value");
      }
    } else if (args[2].isNumber())
      JS_ValueToECMAInt32(cx, args[2], &nStat);
    else {
      JS_free(cx, szTableName);
      THROW_ERROR(cx, "Invalid column value");
    }
    jsval rval;
    FillBaseStat(cx, &rval, nBaseStat, nClassId, nStat, szTableName, szStatName);

    args.rval().set(rval);

    JS_free(cx, szTableName);
    JS_free(cx, szStatName);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_weaponSwitch) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setBoolean(false);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  jsint nParameter = NULL;
  if (args.length() > 0) {
    JSAutoRequest r(cx);
    if (JS_ValueToInt32(cx, args[0], &nParameter) == JS_FALSE) {
      THROW_ERROR(cx, "Could not convert value");
    }
  }

  if (nParameter == NULL) {
    // don't perform a weapon switch if current gametype is classic
    BnetData* pData = (*p_D2LAUNCH_BnData);
    if (pData) {
      if (!(pData->nCharFlags & PLAYER_TYPE_EXPAC)) return JS_TRUE;
    } else
      THROW_ERROR(cx, "Could not acquire BnData");

    BYTE aPacket[1];
    aPacket[0] = 0x60;
    D2NET_SendPacket(1, 1, aPacket);
    args.rval().setBoolean(true);
  } else {
    args.rval().setInt32(*p_D2CLIENT_bWeapSwitch);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_transmute) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setNull();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  bool cubeOn = !!D2CLIENT_GetUIState(UI_CUBE);
  if (!cubeOn) D2CLIENT_SetUIState(UI_CUBE, TRUE);

  D2CLIENT_Transmute();

  if (!cubeOn) D2CLIENT_SetUIState(UI_CUBE, FALSE);

  return JS_TRUE;
}

JSAPI_FUNC(my_getPlayerFlag) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber()) return JS_TRUE;

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  uint32 nFirstUnitId = (uint32)-1;
  uint32 nSecondUnitId = (uint32)-1;
  {
    JSAutoRequest r(cx);
    JS_ValueToECMAUint32(cx, args[0], &nFirstUnitId);
    JS_ValueToECMAUint32(cx, args[1], &nSecondUnitId);
  }

  DWORD nFlag = args[2].toInt32();

  args.rval().setBoolean(D2CLIENT_TestPvpFlag(nFirstUnitId, nSecondUnitId, nFlag));
  return JS_TRUE;
}

JSAPI_FUNC(my_getMouseCoords) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  bool nFlag = false, nReturn = false;

  if (args.length() > 0 && args[0].isInt32() || args[0].isBoolean()) nFlag = args[0].toBoolean();
  if (args.length() > 1 && args[1].isInt32() || args[1].isBoolean()) nReturn = args[1].toBoolean();

  POINT Coords = {static_cast<LONG>(*p_D2CLIENT_MouseX), static_cast<LONG>(*p_D2CLIENT_MouseY)};

  if (nFlag) {
    Coords.x += *p_D2CLIENT_ViewportX;
    Coords.y += *p_D2CLIENT_ViewportY;

    D2COMMON_AbsScreenToMap(&Coords.x, &Coords.y);
  }

  jsval jsX = JS::Int32Value(Coords.x);
  jsval jsY = JS::Int32Value(Coords.y);

  JS::RootedObject pObj(cx);
  if (nReturn) {
    pObj = BuildObject(cx, NULL);
    if (!pObj) THROW_ERROR(cx, "Could not build object");
    if (JS_SetProperty(cx, pObj, "x", &jsX) == JS_FALSE) THROW_ERROR(cx, "Could not set x property");
    if (JS_SetProperty(cx, pObj, "y", &jsY) == JS_FALSE) THROW_ERROR(cx, "Could not set y property");
  } else {
    JSAutoRequest r(cx);
    pObj = JS_NewArrayObject(cx, NULL, NULL);
    JS_SetElement(cx, pObj, 0, &jsX);
    JS_SetElement(cx, pObj, 1, &jsY);
  }

  if (!pObj) return JS_TRUE;

  args.rval().setObjectOrNull(pObj);
  return JS_TRUE;
}

JSAPI_FUNC(my_submitItem) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setNull();
  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (UnitAny* pUnit = D2CLIENT_GetCursorItem()) {
    if (D2CLIENT_GetPlayerUnit()->dwAct == 1) {
      if (GetPlayerArea() == D2CLIENT_GetPlayerUnit()->pAct->pMisc->dwStaffTombLevel) {
        *p_D2CLIENT_CursorItemMode = 3;
        BYTE aPacket[17] = {NULL};
        aPacket[0] = 0x44;
        *(DWORD*)&aPacket[1] = D2CLIENT_GetPlayerUnit()->dwUnitId;
        *(DWORD*)&aPacket[5] = *p_D2CLIENT_OrificeId;
        *(DWORD*)&aPacket[9] = pUnit->dwUnitId;
        *(DWORD*)&aPacket[13] = 3;
        D2NET_SendPacket(17, 1, aPacket);
        args.rval().setBoolean(true);
      } else {
        args.rval().setBoolean(false);
      }
    } else if (D2CLIENT_GetPlayerUnit()->dwAct == 0 || D2CLIENT_GetPlayerUnit()->dwAct == 4)  // dwAct is 0-4, not 1-5
    {
      if (*p_D2CLIENT_RecentInteractId && D2COMMON_IsTownByLevelNo(GetPlayerArea())) {
        D2CLIENT_SubmitItem(pUnit->dwUnitId);
        args.rval().setBoolean(true);
      } else {
        args.rval().setBoolean(false);
      }
    } else {
      args.rval().setBoolean(false);
    }
  } else {
    args.rval().setBoolean(false);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getIsTalkingNPC) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  args.rval().setBoolean(IsScrollingText());
  return JS_TRUE;
}

JSAPI_FUNC(my_getInteractedNPC) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  UnitAny* pNPC = D2CLIENT_GetCurrentInteractingNPC();
  if (!pNPC) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  myUnit* pmyUnit = new myUnit;  // leaked?
  if (!pmyUnit) return JS_TRUE;

  char szName[256] = "";
  pmyUnit->_dwPrivateType = PRIVATE_UNIT;
  pmyUnit->dwClassId = pNPC->dwTxtFileNo;
  pmyUnit->dwMode = pNPC->dwMode;
  pmyUnit->dwType = pNPC->dwType;
  pmyUnit->dwUnitId = pNPC->dwUnitId;
  strcpy_s(pmyUnit->szName, sizeof(pmyUnit->szName), szName);

  JSObject* jsunit = BuildObject(cx, &unit_class, unit_methods, unit_props, pmyUnit);
  if (!jsunit) return JS_TRUE;

  args.rval().setObjectOrNull(jsunit);
  return JS_TRUE;
}

JSAPI_FUNC(my_takeScreenshot) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Vars.bTakeScreenshot = true;

  args.rval().setUndefined();
  return JS_TRUE;
}
JSAPI_FUNC(my_moveNPC) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (!Vars.settings.bEnableUnsupported) {
    THROW_WARNING(cx, vp, "moveNPC requires EnableUnsupported = true in d2bs.ini");
  }

  if (args.length() < 2) THROW_ERROR(cx, "Not enough parameters were passed to moveNPC!");

  args.rval().setBoolean(false);
  myUnit* pNpc = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());

  if (!pNpc || pNpc->dwType != 1) THROW_ERROR(cx, "Invalid NPC passed to moveNPC!");

  DWORD dwX = args[1].toInt32();
  DWORD dwY = args[2].toInt32();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  BYTE aPacket[17];
  aPacket[0] = 0x59;
  *(DWORD*)&aPacket[1] = pNpc->dwType;
  *(DWORD*)&aPacket[5] = pNpc->dwUnitId;
  *(DWORD*)&aPacket[9] = dwX;
  *(DWORD*)&aPacket[13] = dwY;

  D2NET_SendPacket(sizeof(aPacket), 1, aPacket);
  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_revealLevel) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  UnitAny* unit = D2CLIENT_GetPlayerUnit();

  if (!unit) {
    return JS_TRUE;
  }

  Level* level = unit->pPath->pRoom1->pRoom2->pLevel;

  if (!level) {
    return JS_TRUE;
  }

  BOOL bDrawPresets = false;

  if (argc == 1 && args[0].isBoolean()) {
    bDrawPresets = args[0].toBoolean();
  }

  CriticalRoom cRoom;
  if (!GameReady()) {
    return JS_TRUE;
  }

  for (Room2* room = level->pRoom2First; room; room = room->pRoom2Next) {
    RevealRoom(room, bDrawPresets);
  }

  return JS_TRUE;
}
