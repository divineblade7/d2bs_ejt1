#include "d2bs/script/api/JSPresetUnit.h"

#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/utils/CriticalSections.h"

EMPTY_CTOR(presetunit)

void presetunit_finalize(JSFreeOp*, JSObject* obj) {
  myPresetUnit* pUnit = (myPresetUnit*)JS_GetPrivate(obj);

  if (pUnit) {
    JS_SetPrivate(obj, NULL);
    delete pUnit;
  }
}

JSAPI_PROP(presetunit_getProperty) {
  myPresetUnit* pUnit = (myPresetUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  jsval ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case PUNIT_TYPE:
      vp.setInt32(pUnit->dwType);
      break;
    case PUNIT_ROOMX:
      vp.setInt32(pUnit->dwRoomX);
      break;
    case PUNIT_ROOMY:
      vp.setInt32(pUnit->dwRoomY);
      break;
    case PUNIT_X:
      vp.setInt32(pUnit->dwPosX);
      break;
    case PUNIT_Y:
      vp.setInt32(pUnit->dwPosY);
      break;
    case PUNIT_ID:
      vp.setInt32(pUnit->dwId);
      break;
    case PUINT_LEVEL:
      vp.setInt32(pUnit->dwLevel);
    default:
      break;
  }
  return JS_TRUE;
}

JSAPI_FUNC(my_getPresetUnits) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) {
    THROW_WARNING(cx, vp, "Game not ready");
  }

  if (args.length() < 1) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  uint32 levelId = args[0].toInt32();
  Level* pLevel = GetLevel(levelId);

  if (!pLevel) {
    THROW_ERROR(cx, "getPresetUnits failed, couldn't access the level!");
  }

  uint nClassId = NULL;
  uint nType = NULL;

  if (args.length() >= 2 && !args[1].isUndefined()) {
    nType = args.get(1).toInt32();
  }
  // BUG: For some reason we get argc == 3 even if only 2 args is supplied!
  if (args.length() >= 3 && !args[2].isUndefined()) {
    nClassId = args.get(2).toInt32();
  }

  CriticalRoom cRoom;

  bool bAddedRoom = FALSE;
  DWORD dwArrayCount = NULL;

  JSAutoRequest r(cx);
  JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));
  for (Room2* pRoom = pLevel->pRoom2First; pRoom; pRoom = pRoom->pRoom2Next) {
    bAddedRoom = FALSE;

    if (!pRoom->pPreset) {
      D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                           D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
      bAddedRoom = TRUE;
    }

    for (PresetUnit* pUnit = pRoom->pPreset; pUnit; pUnit = pUnit->pPresetNext) {
      // Does it fit?
      if ((nType == NULL || pUnit->dwType == nType) && (nClassId == NULL || pUnit->dwTxtFileNo == nClassId)) {
        myPresetUnit* mypUnit = new myPresetUnit;

        mypUnit->dwPosX = pUnit->dwPosX;
        mypUnit->dwPosY = pUnit->dwPosY;
        mypUnit->dwRoomX = pRoom->dwPosX;
        mypUnit->dwRoomY = pRoom->dwPosY;
        mypUnit->dwType = pUnit->dwType;
        mypUnit->dwId = pUnit->dwTxtFileNo;
        mypUnit->dwLevel = levelId;

        JSObject* unit = BuildObject(cx, &presetunit_class, NULL, presetunit_props, mypUnit);
        if (!unit) {
          delete mypUnit;
          THROW_ERROR(cx, "Failed to build object?");
        }

        jsval a = JS::ObjectOrNullValue(unit);
        JS_SetElement(cx, pReturnArray, dwArrayCount, &a);

        dwArrayCount++;
      }
    }

    if (bAddedRoom) {
      D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                              D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
      bAddedRoom = FALSE;
    }
  }

  args.rval().setObjectOrNull(pReturnArray);
  return JS_TRUE;
}

JSAPI_FUNC(my_getPresetUnit) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  if (args.length() < 1) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  uint32 levelId;
  {
    JSAutoRequest r(cx);
    JS_ValueToECMAUint32(cx, args[0], &levelId);
  }
  Level* pLevel = GetLevel(levelId);

  if (!pLevel) THROW_ERROR(cx, "getPresetUnits failed, couldn't access the level!");

  DWORD nClassId = NULL;
  DWORD nType = NULL;

  if (args.length() >= 2) nType = args[1].toInt32();
  if (args.length() >= 3) nClassId = args[2].toInt32();

  CriticalRoom cRoom;

  bool bAddedRoom = FALSE;

  for (Room2* pRoom = pLevel->pRoom2First; pRoom; pRoom = pRoom->pRoom2Next) {
    bAddedRoom = FALSE;

    if (!pRoom->pRoom1) {
      D2COMMON_AddRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                           D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
      bAddedRoom = TRUE;
    }

    for (PresetUnit* pUnit = pRoom->pPreset; pUnit; pUnit = pUnit->pPresetNext) {
      // Does it fit?
      if ((nType == NULL || pUnit->dwType == nType) && (nClassId == NULL || pUnit->dwTxtFileNo == nClassId)) {
        // Yes it fits! Return it
        myPresetUnit* mypUnit = new myPresetUnit;

        mypUnit->dwPosX = pUnit->dwPosX;
        mypUnit->dwPosY = pUnit->dwPosY;
        mypUnit->dwRoomX = pRoom->dwPosX;
        mypUnit->dwRoomY = pRoom->dwPosY;
        mypUnit->dwType = pUnit->dwType;
        mypUnit->dwId = pUnit->dwTxtFileNo;
        mypUnit->dwLevel = levelId;

        JSObject* obj = BuildObject(cx, &presetunit_class, NULL, presetunit_props, mypUnit);
        if (!obj) {
          delete mypUnit;
          THROW_ERROR(cx, "Failed to create presetunit object");
        }
        args.rval().setObjectOrNull(obj);
        return JS_TRUE;
      }
    }

    if (bAddedRoom) {
      D2COMMON_RemoveRoomData(D2CLIENT_GetPlayerUnit()->pAct, pLevel->dwLevelNo, pRoom->dwPosX, pRoom->dwPosY,
                              D2CLIENT_GetPlayerUnit()->pPath->pRoom1);
      bAddedRoom = FALSE;
    }
  }

  args.rval().setBoolean(false);
  return JS_TRUE;
}
