#include "d2bs/script/api/JSArea.h"

#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/script/api/JSExits.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/utils/CriticalSections.h"
#include "d2bs/utils/MapHeader.h"

EMPTY_CTOR(area)

void area_finalize(JSFreeOp*, JSObject* obj) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (pArea) {
    JS_SetPrivate(obj, NULL);
    delete pArea;
  }
}

JSAPI_FUNC(my_getArea) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS_SET_RVAL(cx, vp, JSVAL_VOID);

  if (!WaitForGameReady()) {
    THROW_ERROR(cx, "Get Area: Game not ready");
  }

  int32_t nArea = GetPlayerArea();
  if (args.length() == 1) {
    if (args[0].isInt32()) {
      JSAutoRequest r(cx);
      JS_ValueToECMAInt32(cx, JS_ARGV(cx, vp)[0], &nArea);
    } else {
      THROW_ERROR(cx, "Invalid parameter passed to getArea!");
    }
  }

  if (nArea < 0) {
    THROW_ERROR(cx, "Invalid parameter passed to getArea!");
  }

  Level* pLevel = GetLevel(nArea);

  if (!pLevel) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  ApiArea* pArea = new ApiArea;
  if (!pArea) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  pArea->AreaId = nArea;
  pArea->ExitArray = NULL;

  JSObject* unit = BuildObject(cx, &area_class, NULL, area_props, pArea);
  if (!unit) {
    delete pArea;
    pArea = NULL;
    THROW_ERROR(cx, "Failed to build area unit!");
  }

  args.rval().setObjectOrNull(unit);
  return JS_TRUE;
}

JSAPI_PROP(area_id) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  vp.setInt32(pLevel->dwLevelNo);
  return JS_TRUE;
}

JSAPI_PROP(area_name) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  LevelTxt* pTxt = D2COMMON_GetLevelText(pArea->AreaId);
  if (pTxt) vp.setString(JS_InternString(cx, pTxt->szName));
  return JS_TRUE;
}

JSAPI_PROP(area_x) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  vp.setInt32(pLevel->dwPosX);
  return JS_TRUE;
}

JSAPI_PROP(area_y) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  vp.setInt32(pLevel->dwPosY);
  return JS_TRUE;
}

JSAPI_PROP(area_xsize) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  vp.setInt32(pLevel->dwSizeX);
  return JS_TRUE;
}

JSAPI_PROP(area_ysize) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  vp.setInt32(pLevel->dwSizeY);
  return JS_TRUE;
}

JSAPI_PROP(area_exits) {
  ApiArea* pArea = (ApiArea*)JS_GetPrivate(obj);

  if (!pArea) return JS_FALSE;

  Level* pLevel = GetLevel(pArea->AreaId);
  if (!pLevel) return JS_FALSE;

  JSAutoRequest r(cx);
  JS::RootedObject exitArray(cx);
  if (pArea->ExitArray == NULL) {
    exitArray = JS_NewArrayObject(cx, 0, nullptr);

    ActMap* map = ActMap::GetMap(pLevel);

    ExitArray exits;
    map->GetExits(exits);
    map->CleanUp();
    int count = exits.size();
    for (int i = 0; i < count; i++) {
      myExit* exit = new myExit;
      exit->id = exits[i].Target;
      exit->x = exits[i].Position.first;
      exit->y = exits[i].Position.second;
      exit->type = exits[i].Type;
      exit->tileid = exits[i].TileId;
      exit->level = pArea->AreaId;

      JSObject* pExit = BuildObject(cx, &exit_class, NULL, exit_props, exit);
      if (!pExit) {
        delete exit;
        THROW_ERROR(cx, "Failed to create exit object!");
      }
      JS::Value a = OBJECT_TO_JSVAL(pExit);
      JS_SetElement(cx, exitArray, i, &a);
    }

    pArea->ExitArray = exitArray;
  }
  vp.setObjectOrNull(exitArray);
  return JS_TRUE;
}
