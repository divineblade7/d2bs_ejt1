#include "d2bs/script/api/JSParty.h"

#include "d2bs/core/Unit.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/D2Structs.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSUnit.h"

EMPTY_CTOR(party)

JSAPI_PROP(party_gid) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setNumber((double)pUnit->dwUnitId);
  return JS_TRUE;
}

JSAPI_PROP(party_name) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setString(JS_NewStringCopyZ(cx, pUnit->szName));
  return JS_TRUE;
}

JSAPI_PROP(party_classid) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->dwClassId);
  return JS_TRUE;
}

JSAPI_PROP(party_area) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->dwLevelId);
  return JS_TRUE;
}

JSAPI_PROP(party_level) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->wLevel);
  return JS_TRUE;
}

JSAPI_PROP(party_x) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->Xpos);
  return JS_TRUE;
}

JSAPI_PROP(party_y) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->Ypos);
  return JS_TRUE;
}

JSAPI_PROP(party_life) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->dwPartyLife);
  return JS_TRUE;
}

JSAPI_PROP(party_partyflag) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->dwPartyFlags);
  return JS_TRUE;
}

JSAPI_PROP(party_partyid) {
  RosterUnit* pUnit = (RosterUnit*)JS_GetPrivate(obj);

  if (!pUnit) return JS_TRUE;

  vp.setInt32(pUnit->wPartyId);
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
