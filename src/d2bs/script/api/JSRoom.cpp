#include "d2bs/script/api/JSRoom.h"

#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/D2Structs.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSPresetUnit.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/utils/CriticalSections.h"

EMPTY_CTOR(room)

JSAPI_PROP(room_getProperty) {
  Room2* pRoom2 = (Room2*)JS_GetPrivate(obj);

  if (!pRoom2) return JS_TRUE;
  jsval ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case ROOM_NUM:
      if (pRoom2->dwPresetType != 2)
        vp.setInt32(-1);
      else if (pRoom2->pType2Info)
        vp.setInt32(pRoom2->pType2Info->dwRoomNumber);
      break;
    case ROOM_XPOS:
      vp.setInt32(pRoom2->dwPosX);
      break;
    case ROOM_YPOS:
      vp.setInt32(pRoom2->dwPosY);
      break;
    case ROOM_XSIZE:
      vp.setInt32(pRoom2->dwSizeX * 5);
      break;
    case ROOM_YSIZE:
      vp.setInt32(pRoom2->dwSizeY * 5);
      break;
    case ROOM_AREA:
    case ROOM_LEVEL:
      if (pRoom2->pLevel) vp.setInt32(pRoom2->pLevel->dwLevelNo);
      break;

    case ROOM_CORRECTTOMB:
      if (pRoom2->pLevel && pRoom2->pLevel->pMisc && pRoom2->pLevel->pMisc->dwStaffTombLevel)
        vp.setInt32(pRoom2->pLevel->pMisc->dwStaffTombLevel);
      break;

    default:
      break;
  }

  return JS_TRUE;
}

JSAPI_FUNC(room_getNext) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self.toObjectOrNull());
  if (!pRoom2) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  pRoom2 = pRoom2->pRoom2Next;

  if (!pRoom2) {
    JSObject* obj = self.toObjectOrNull();
    //		JS_ClearScope(cx, obj);
    if (JS_ValueToObject(cx, JSVAL_NULL, &obj)) {
      args.rval().setBoolean(false);
    }
  } else {
    JS_SetPrivate(self.toObjectOrNull(), pRoom2);
    args.rval().setBoolean(true);
  }

  return JS_TRUE;
}

JSAPI_FUNC(room_getPresetUnits) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);

  DWORD nType = NULL;
  DWORD nClass = NULL;

  if (args.length() > 0 && args[0].isInt32()) nType = args[0].toInt32();
  if (args.length() > 1 && args[1].isInt32()) nClass = args[1].toInt32();

  bool bAdded = FALSE;
  DWORD dwArrayCount = NULL;

  CriticalRoom cRoom;
  if (!pRoom2 || !GameReady()) {
    return JS_TRUE;
  }

  if (!pRoom2->pRoom1) {
    bAdded = TRUE;
    D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                         D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
  }

  JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));
  for (PresetUnit* pUnit = pRoom2->pPreset; pUnit; pUnit = pUnit->pPresetNext) {
    if ((pUnit->dwType == nType || nType == NULL) && (pUnit->dwTxtFileNo == nClass || nClass == NULL)) {
      myPresetUnit* mypUnit = new myPresetUnit;

      mypUnit->dwPosX = pUnit->dwPosX;
      mypUnit->dwPosY = pUnit->dwPosY;
      mypUnit->dwRoomX = pRoom2->dwPosX;
      mypUnit->dwRoomY = pRoom2->dwPosY;
      mypUnit->dwType = pUnit->dwType;
      mypUnit->dwId = pUnit->dwTxtFileNo;

      JSObject* jsUnit = BuildObject(cx, &presetunit_class, NULL, presetunit_props, mypUnit);
      if (!jsUnit) {
        args.rval().setBoolean(false);
        return JS_TRUE;
      }

      jsval a = JS::ObjectOrNullValue(jsUnit);
      JSAutoRequest r(cx);
      JS_SetElement(cx, pReturnArray, dwArrayCount, &a);
      dwArrayCount++;
    }
  }

  if (bAdded)
    D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                            D2CLIENT_GetPlayerUnit()->pPath->pRoom1);

  args.rval().setObjectOrNull(pReturnArray);
  return JS_TRUE;
}

JSAPI_FUNC(room_getCollisionTypeArray) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);

  bool bAdded = FALSE;
  CollMap* pCol = NULL;

  CriticalRoom cRoom;
  if (!pRoom2 || !GameReady()) {
    return JS_TRUE;
  }

  if (!pRoom2->pRoom1) {
    bAdded = TRUE;
    D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                         D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
  }

  JS::RootedObject jsobjy(cx, JS_NewUint16Array(cx, (pRoom2->dwSizeX * 5) * (pRoom2->dwSizeY * 5)));

  if (!jsobjy) return JS_TRUE;

  if (pRoom2->pRoom1) pCol = pRoom2->pRoom1->Coll;

  if (!pCol) {
    if (bAdded)
      D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                              D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
    return JS_TRUE;
  }

  int x = pCol->dwPosGameX - pRoom2->pLevel->dwPosX * 5;
  int y = pCol->dwPosGameY - pRoom2->pLevel->dwPosY * 5;
  int nCx = pCol->dwSizeGameX;
  int nCy = pCol->dwSizeGameY;

  int nLimitX = x + nCx;
  int nLimitY = y + nCy;

  int nCurrentArrayY = NULL;

  WORD* p = pCol->pMapStart;
  JSAutoRequest r(cx);
  for (int j = y; j < nLimitY; j++) {
    int nCurrentArrayX = 0;
    for (int i = x; i < nLimitX; i++) {
      jsval nNode = JS::Int32Value(*p);

      if (!JS_SetElement(cx, jsobjy, nCurrentArrayY * nCx + nCurrentArrayX, &nNode)) {
        if (bAdded)
          D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX,
                                  pRoom2->dwPosY, D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
        return JS_TRUE;
      }

      nCurrentArrayX++;
      p++;
    }
    nCurrentArrayY++;
  }

  if (bAdded)
    D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                            D2CLIENT_GetPlayerUnit()->pPath->pRoom1);

  args.rval().setObjectOrNull(jsobjy);
  return JS_TRUE;
}

JSAPI_FUNC(room_getCollision) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);

  bool bAdded = FALSE;
  CollMap* pCol = NULL;

  CriticalRoom cRoom;
  if (!pRoom2 || !GameReady()) {
    return JS_TRUE;
  }

  JS::RootedObject jsobjy(cx, JS_NewArrayObject(cx, NULL, NULL));
  if (!jsobjy) return JS_TRUE;

  if (!pRoom2->pRoom1) {
    bAdded = TRUE;
    D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                         D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
  }

  if (pRoom2->pRoom1) pCol = pRoom2->pRoom1->Coll;

  if (!pCol) {
    if (bAdded)
      D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                              D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
    return JS_TRUE;
  }

  int x = pCol->dwPosGameX - pRoom2->pLevel->dwPosX * 5;
  int y = pCol->dwPosGameY - pRoom2->pLevel->dwPosY * 5;
  int nCx = pCol->dwSizeGameX;
  int nCy = pCol->dwSizeGameY;

  int nLimitX = x + nCx;
  int nLimitY = y + nCy;

  int nCurrentArrayY = NULL;

  WORD* p = pCol->pMapStart;
  JSAutoRequest r(cx);
  for (int j = y; j < nLimitY; j++) {
    JSObject* jsobjx = JS_NewArrayObject(cx, NULL, NULL);

    int nCurrentArrayX = 0;
    for (int i = x; i < nLimitX; i++) {
      jsval nNode = JS::Int32Value(*p);

      if (!JS_SetElement(cx, jsobjx, nCurrentArrayX, &nNode)) {
        if (bAdded)
          D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX,
                                  pRoom2->dwPosY, D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
        return JS_TRUE;
      }

      nCurrentArrayX++;
      p++;
    }

    jsval jsu = JS::ObjectOrNullValue(jsobjx);

    if (!JS_SetElement(cx, jsobjy, nCurrentArrayY, &jsu)) {
      if (bAdded)
        D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX,
                                pRoom2->dwPosY, D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
      return JS_TRUE;
    }
    nCurrentArrayY++;
  }

  if (bAdded)
    D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                            D2CLIENT_GetPlayerUnit()->pPath->pRoom1);

  args.rval().setObjectOrNull(jsobjy);
  return JS_TRUE;
}

JSAPI_FUNC(room_getNearby) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);
  JSObject* jsobj = JS_NewArrayObject(cx, NULL, NULL);

  if (!jsobj) return JS_TRUE;

  JSAutoRequest r(cx);
  for (DWORD i = 0; i < pRoom2->dwRoomsNear; ++i) {
    JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props, pRoom2->pRoom2Near[i]);
    if (!jsroom) {
      return JS_TRUE;
    }
    jsval jsu = JS::ObjectOrNullValue(jsroom);

    if (!JS_SetElement(cx, jsobj, i, &jsu)) {
      return JS_TRUE;
    }
  }

  args.rval().setObjectOrNull(jsobj);
  return JS_TRUE;
}

// Don't know whether it works or not
JSAPI_FUNC(room_getStat) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);

  args.rval().setNull();
  if (argc < 1 || !args[0].isInt32()) return JS_TRUE;

  jsint nStat = args[0].toInt32();

  bool bAdded = false;

  CriticalRoom cRoom;
  if (!pRoom2 || !GameReady()) {
    return JS_TRUE;
  }
  if (!pRoom2->pRoom1) {
    bAdded = true;
    D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                         D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
  }

  if (!pRoom2->pRoom1) {
    return JS_TRUE;
  }

  switch (nStat) {
    case 0:
      args.rval().setInt32(pRoom2->pRoom1->dwXStart);
      break;
    case 1:
      args.rval().setInt32(pRoom2->pRoom1->dwYStart);
      break;
    case 2:
      args.rval().setInt32(pRoom2->pRoom1->dwXSize);
      break;
    case 3:
      args.rval().setInt32(pRoom2->pRoom1->dwYSize);
      break;
    case 4:
      args.rval().setInt32(pRoom2->dwPosX);
      break;
    case 5:
      args.rval().setInt32(pRoom2->dwPosY);
      break;
    case 6:
      args.rval().setInt32(pRoom2->dwSizeX);
      break;
    case 7:
      args.rval().setInt32(pRoom2->dwSizeY);
      break;
    case 8:
      // args.rval().setInt32(pRoom2->dwYStart);
      break;
    case 9:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwPosGameX);
      break;
    case 10:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwPosGameY);
      break;
    case 11:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwSizeGameX);
      break;
    case 12:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwSizeGameY);
      break;
    case 13:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwPosRoomX);
      break;
    case 14:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwPosGameY);
      break;
    case 15:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwSizeRoomX);
      break;
    case 16:
      args.rval().setInt32(pRoom2->pRoom1->Coll->dwSizeRoomY);
      break;
  }

  if (bAdded)
    D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pRoom2->pLevel->dwLevelNo, pRoom2->dwPosX, pRoom2->dwPosY,
                            D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
  return JS_TRUE;
}

JSAPI_FUNC(room_getFirst) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);
  if (!pRoom2 || !pRoom2->pLevel || !pRoom2->pLevel->pRoom2First) return JS_TRUE;

  JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props, pRoom2->pLevel->pRoom2First);
  if (!jsroom) return JS_TRUE;

  args.rval().setObjectOrNull(jsroom);

  return JS_TRUE;
}

JSAPI_FUNC(room_unitInRoom) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);
  if (!pRoom2 || args.length() < 1 || !args[0].isObject()) return JS_TRUE;

  myUnit* pmyUnit = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());

  if (!pmyUnit || (pmyUnit->_dwPrivateType & PRIVATE_UNIT) != PRIVATE_UNIT) return JS_TRUE;

  UnitAny* pUnit = D2CLIENT_FindUnit(pmyUnit->dwUnitId, pmyUnit->dwType);

  if (!pUnit) return JS_TRUE;

  Room1* pRoom1 = D2COMMON_GetRoomFromUnit(pUnit);

  if (!pRoom1 || !pRoom1->pRoom2) return JS_TRUE;

  args.rval().setBoolean(pRoom1->pRoom2 == pRoom2);
  return JS_TRUE;
}

JSAPI_FUNC(room_reveal) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Room2* pRoom2 = (Room2*)JS_GetPrivate(self);

  BOOL bDrawPresets = false;
  if (args.length() == 1 && args[0].isBoolean()) bDrawPresets = args[0].toBoolean();

  CriticalRoom cRoom;
  if (!pRoom2 || !GameReady()) {
    return JS_TRUE;
  }

  args.rval().setBoolean(RevealRoom(pRoom2, bDrawPresets));
  return JS_TRUE;
}

JSAPI_FUNC(my_getRoom) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (!WaitForGameReady()) THROW_ERROR(cx, "Get Room Game not ready");

  CriticalRoom cRoom;

  if (args.length() == 1 && args[0].isInt32()) {
    uint32 levelId;
    {
      JSAutoRequest r(cx);
      JS_ValueToECMAUint32(cx, args[0], &levelId);
    }
    if (levelId != 0)  // 1 Parameter, AreaId
    {
      Level* pLevel = GetLevel(levelId);

      if (!pLevel || !pLevel->pRoom2First) {
        return JS_TRUE;
      }

      JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props, pLevel->pRoom2First);
      if (!jsroom) {
        return JS_TRUE;
      }
      args.rval().setObjectOrNull(jsroom);
      return JS_TRUE;
    } else if (levelId == 0) {
      Room1* pRoom1 = D2COMMON_GetRoomFromUnit(D2CLIENT_GetPlayerUnit());

      if (!pRoom1 || !pRoom1->pRoom2) {
        return JS_TRUE;
      }

      JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props, pRoom1->pRoom2);
      if (!jsroom) return JS_TRUE;

      args.rval().setObjectOrNull(jsroom);
      return JS_TRUE;
    }
  } else if (args.length() == 3 || args.length() == 2)  // area ,x and y
  {
    Level* pLevel = NULL;

    uint32 levelId;
    {
      JSAutoRequest r(cx);
      JS_ValueToECMAUint32(cx, args[0], &levelId);
    }
    if (args.length() == 3)
      pLevel = GetLevel(levelId);
    else if (D2CLIENT_GetPlayerUnit() && D2CLIENT_GetPlayerUnit()->pPath && D2CLIENT_GetPlayerUnit()->pPath->pRoom1 &&
             D2CLIENT_GetPlayerUnit()->pPath->pRoom1->pRoom2)
      pLevel = D2CLIENT_GetPlayerUnit()->pPath->pRoom1->pRoom2->pLevel;

    if (!pLevel || !pLevel->pRoom2First) {
      return JS_TRUE;
    }

    uint32 nX = NULL;
    uint32 nY = NULL;
    {
      JSAutoRequest r(cx);
      if (args.length() == 2) {
        JS_ValueToECMAUint32(cx, args[0], &nX);
        JS_ValueToECMAUint32(cx, args[1], &nY);
      } else if (args.length() == 3) {
        JS_ValueToECMAUint32(cx, args[1], &nX);
        JS_ValueToECMAUint32(cx, args[2], &nY);
      }
    }
    if (!nX || !nY) {
      return JS_TRUE;
    }

    // Scan for the room with the matching x,y coordinates.
    for (Room2* pRoom = pLevel->pRoom2First; pRoom; pRoom = pRoom->pRoom2Next) {
      bool bAdded = FALSE;
      if (!pRoom->pRoom1) {
        D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                             D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
        bAdded = TRUE;
      }

      CollMap* map = pRoom->pRoom1->Coll;
      if (nX >= map->dwPosGameX && nY >= map->dwPosGameY && nX < (map->dwPosGameX + map->dwSizeGameX) &&
          nY < (map->dwPosGameY + map->dwSizeGameY)) {
        if (bAdded)
          D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                                  D2CLIENT_GetPlayerUnit()->pPath->pRoom1);

        JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props, pRoom);
        if (!jsroom) return JS_TRUE;

        args.rval().setObjectOrNull(jsroom);
        return JS_TRUE;
      }

      if (bAdded)
        D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                                D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
    }

    JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props, pLevel->pRoom2First);
    if (!jsroom) return JS_TRUE;

    args.rval().setObjectOrNull(jsroom);
    return JS_TRUE;
  } else {
    JSObject* jsroom = BuildObject(cx, &room_class, room_methods, room_props,
                                   D2CLIENT_GetPlayerUnit()->pPath->pRoom1->pRoom2->pLevel->pRoom2First);
    if (!jsroom) return JS_TRUE;

    args.rval().setObjectOrNull(jsroom);
    return JS_TRUE;
  }
  return JS_TRUE;
}
