#include "d2bs/script/api/JSParty.h"

#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/D2Structs.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSUnit.h"

EMPTY_CTOR(party)

JSAPI_PROP(party_getProperty) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  jsval ID;
  JS_IdToValue(cx, id, &ID);
  JS_BeginRequest(cx);
  switch (JSVAL_TO_INT(ID)) {
    case PARTY_NAME:
      vp.setString(JS_NewStringCopyZ(cx, pUnit->szName));
      break;
    case PARTY_X:
      vp.setInt32(pUnit->Xpos);
      break;
    case PARTY_Y:
      vp.setInt32(pUnit->Ypos);
      break;
    case PARTY_AREA:
      vp.setInt32(pUnit->dwLevelId);
      break;
    case PARTY_GID:
      vp.setNumber((double)pUnit->dwUnitId);
      break;
    case PARTY_LIFE:
      vp.setInt32(pUnit->dwPartyLife);
      break;
    case PARTY_CLASSID:
      vp.setInt32(pUnit->dwClassId);
      break;
    case PARTY_LEVEL:
      vp.setInt32(pUnit->wLevel);
      break;
    case PARTY_FLAG:
      vp.setInt32(pUnit->dwPartyFlags);
      break;
    case PARTY_ID:
      vp.setInt32(pUnit->wPartyId);
      break;
    default:
      break;
  }
  JS_EndRequest(cx);
  return JS_TRUE;
}

JSAPI_FUNC(party_getNext) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  auto self = args.thisv();
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(self.toObjectOrNull());

  if (!pUnit) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  pUnit = pUnit->pNext;

  if (pUnit) {
    JS_SetPrivate(self.toObjectOrNull(), pUnit);
    args.rval().setObjectOrNull(self.toObjectOrNull());
  } else {
    args.rval().setBoolean(false);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getParty) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (!WaitForGameReady()) THROW_WARNING(cx, vp, "Game not ready");

  RosterUnit* pUnit = *p_D2CLIENT_PlayerUnitList;

  if (!pUnit) return JS_TRUE;

  if (args.length() == 1) {
    UnitAny* inUnit = NULL;
    char* nPlayerName = nullptr;
    uint32_t nPlayerId = NULL;

    if (args[0].isString()) {
      nPlayerName = JS_EncodeStringToUTF8(cx, args[0].toString());
    } else if (args[0].isInt32()) {
      JSAutoRequest r(cx);
      if (!JS_ConvertArguments(cx, argc, args.array(), "u", &nPlayerId)) {
        THROW_ERROR(cx, "Unable to get ID");
      }
    } else if (args[0].isObject()) {
      myUnit* lpUnit = (myUnit*)JS_GetPrivate(args[0].toObjectOrNull());

      if (!lpUnit) return JS_TRUE;

      inUnit = D2CLIENT_FindUnit(lpUnit->dwUnitId, lpUnit->dwType);
      if (!inUnit) THROW_ERROR(cx, "Unable to get Unit");

      nPlayerId = inUnit->dwUnitId;
    }

    if (!nPlayerName && !nPlayerId) return JS_TRUE;

    BOOL bFound = FALSE;

    for (RosterUnit* pScan = pUnit; pScan; pScan = pScan->pNext) {
      if (nPlayerId && pScan->dwUnitId == nPlayerId) {
        bFound = TRUE;
        pUnit = pScan;
        break;
      }
      if (nPlayerName && _stricmp(pScan->szName, nPlayerName) == 0) {
        bFound = TRUE;
        pUnit = pScan;
        break;
      }
    }

    JS_free(cx, nPlayerName);

    if (!bFound) return JS_TRUE;
  }

  JSObject* jsUnit = BuildObject(cx, &party_class, party_methods, party_props, pUnit);
  if (!jsUnit) return JS_TRUE;

  args.rval().setObjectOrNull(jsUnit);

  return JS_TRUE;
}
