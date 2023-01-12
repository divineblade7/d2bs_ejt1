// #include "Control.h"
#include "d2bs/script/api/JSControl.h"

#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/utils/Helpers.h"

EMPTY_CTOR(control)

void control_finalize(JSFreeOp*, JSObject* obj) {
  ControlData* pData = ((ControlData*)JS_GetPrivate(obj));

  if (pData) {
    JS_SetPrivate(obj, NULL);
    delete pData;
  }
}

JSAPI_PROP(control_getProperty) {
  if (ClientState() != ClientStateMenu) {
    return JS_FALSE;
  }

  ControlData* pData = ((ControlData*)JS_GetPrivate(obj));
  if (!pData) return JS_FALSE;

  Control* ctrl =
      findControl(pData->dwType, (const wchar_t*)NULL, -1, pData->dwX, pData->dwY, pData->dwSizeX, pData->dwSizeY);
  if (!ctrl) return JS_FALSE;

  JS::Value ID;
  JS_IdToValue(cx, id, &ID);
  JS_BeginRequest(cx);

  // JSType a = JS_TypeOfValue(cx, ID);

  if (JSID_IS_STRING(id)) {
    // TODO: Figure out what this is for
    // JSString* b = JSVAL_TO_STRING(ID);
    // char* pText = JS_EncodeString(cx, b);

    return JS_TRUE;
  }
  if (JSID_IS_VOID(id)) return JS_TRUE;
  if (JSID_IS_ZERO(id)) return JS_TRUE;
  switch (JSID_TO_INT(id)) {
    case CONTROL_TEXT:
      if (ctrl->dwIsCloaked != 33) {
        vp.setString(JS_InternUCString(cx, ctrl->dwType == 6 ? ctrl->wText2 : ctrl->wText));
      }
      break;
    case CONTROL_X:
      vp.setNumber((double)ctrl->dwPosX);
      // JS_NewNumberValue(cx, ctrl->dwPosX, vp);
      break;
    case CONTROL_Y:
      vp.setNumber((double)ctrl->dwPosY);
      break;
    case CONTROL_XSIZE:
      vp.setNumber((double)ctrl->dwSizeX);
      break;
    case CONTROL_YSIZE:
      vp.setNumber((double)ctrl->dwSizeY);
      break;
    case CONTROL_STATE:
      vp.setNumber((double)(ctrl->dwDisabled - 2));
      break;
    case CONTROL_MAXLENGTH:
      // JS_NewNumberValue(cx, ctrl->dwMaxLength, vp);
      break;
    case CONTROL_TYPE:
      vp.setNumber((double)ctrl->dwType);
      break;
    case CONTROL_VISIBLE:
      // nothing to do yet because we don't know what to do
      break;
    case CONTROL_CURSORPOS:
      vp.setNumber((double)ctrl->dwCursorPos);
      break;
    case CONTROL_SELECTSTART:
      vp.setNumber((double)ctrl->dwSelectStart);
      break;
    case CONTROL_SELECTEND:
      vp.setNumber((double)ctrl->dwSelectEnd);
      break;
    case CONTROL_PASSWORD:
      vp.setBoolean(!!(ctrl->dwIsCloaked == 33));
      break;
    case CONTROL_DISABLED:
      vp.setNumber((double)ctrl->dwDisabled);
      break;
  }
  JS_EndRequest(cx);
  return JS_TRUE;
}

JSAPI_STRICT_PROP(control_setProperty) {
  if (ClientState() != ClientStateMenu) return JS_FALSE;

  ControlData* pData = ((ControlData*)JS_GetPrivate(obj));  // JS_THIS_OBJECT(cx, &vp.get())));
  if (!pData) return JS_FALSE;

  Control* ctrl =
      findControl(pData->dwType, (const wchar_t*)NULL, -1, pData->dwX, pData->dwY, pData->dwSizeX, pData->dwSizeY);
  if (!ctrl) return JS_FALSE;

  JS::Value ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case CONTROL_TEXT:
      if (ctrl->dwType == 1 && vp.isString()) {
        const wchar_t* szwText = JS_GetStringCharsZ(cx, vp.toString());
        if (!szwText) return JS_TRUE;
        D2WIN_SetControlText(ctrl, szwText);
      }
      break;
    case CONTROL_STATE:
      if (vp.isInt32()) {
        int32_t nState;
        JS_BeginRequest(cx);
        if (!JS_ValueToECMAInt32(cx, vp.get(), &nState) || nState < 0 || nState > 3) {
          JS_EndRequest(cx);
          THROW_ERROR(cx, "Invalid state value");
        }
        memset((void*)&ctrl->dwDisabled, (nState + 2), sizeof(DWORD));
        JS_EndRequest(cx);
      }
      break;
    case CONTROL_CURSORPOS:
      if (vp.isInt32()) {
        JS_BeginRequest(cx);
        uint32_t dwPos;
        if (!JS_ValueToECMAUint32(cx, vp.get(), &dwPos)) {
          JS_EndRequest(cx);
          THROW_ERROR(cx, "Invalid cursor position value");
        }
        JS_EndRequest(cx);
        memset((void*)&ctrl->dwCursorPos, dwPos, sizeof(DWORD));
      }
      break;
    case CONTROL_DISABLED:
      if (vp.isInt32()) {
        memset((void*)&ctrl->dwDisabled, vp.toInt32(), sizeof(DWORD));
      }
      break;
  }

  return JS_TRUE;
}

JSAPI_FUNC(control_getNext) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (ClientState() != ClientStateMenu) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  auto self = args.thisv();
  ControlData* pData = ((ControlData*)JS_GetPrivate(self.toObjectOrNull()));
  if (!pData) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  Control* pControl =
      findControl(pData->dwType, (const wchar_t*)NULL, -1, pData->dwX, pData->dwY, pData->dwSizeX, pData->dwSizeY);
  if (pControl && pControl->pNext)
    pControl = pControl->pNext;
  else
    pControl = NULL;

  if (pControl) {
    pData->pControl = pControl;
    pData->dwType = pData->pControl->dwType;
    pData->dwX = pData->pControl->dwPosX;
    pData->dwY = pData->pControl->dwPosY;
    pData->dwSizeX = pData->pControl->dwSizeX;
    pData->dwSizeY = pData->pControl->dwSizeY;

    JS_SetPrivate(self.toObjectOrNull(), pData);
    args.rval().setObjectOrNull(self.toObjectOrNull());
  } else {
    args.rval().setBoolean(false);
  }

  return JS_TRUE;
}

JSAPI_FUNC(control_click) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (ClientState() != ClientStateMenu) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  auto self = args.thisv();
  ControlData* pData = ((ControlData*)JS_GetPrivate(self.toObjectOrNull()));
  if (!pData) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  Control* pControl =
      findControl(pData->dwType, (const wchar_t*)NULL, -1, pData->dwX, pData->dwY, pData->dwSizeX, pData->dwSizeY);
  if (!pControl) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  uint32_t x = (uint32_t)-1, y = (uint32_t)-1;

  if (args.length() > 1 && args[0].isInt32() && args[1].isInt32()) {
    JSAutoRequest r(cx);
    JS_ValueToECMAUint32(cx, args[0], &x);
    JS_ValueToECMAUint32(cx, args[1], &y);
  }

  clickControl(pControl, x, y);

  return JS_TRUE;
}

JSAPI_FUNC(control_setText) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) {
    return JS_TRUE;
  }

  auto self = args.thisv();
  ControlData* pData = ((ControlData*)JS_GetPrivate(self.toObjectOrNull()));
  if (!pData) {
    return JS_TRUE;
  }

  Control* pControl =
      findControl(pData->dwType, (const wchar_t*)NULL, -1, pData->dwX, pData->dwY, pData->dwSizeX, pData->dwSizeY);
  if (!pControl) {
    return JS_TRUE;
  }

  if (args.length() < 1 || !args[0].isString()) {
    return JS_TRUE;
  }

  const wchar_t* szwText = JS_GetStringCharsZ(cx, JS_ValueToString(cx, args[0]));
  if (!szwText) {
    return JS_TRUE;
  }

  D2WIN_SetControlText(pControl, szwText);
  return JS_TRUE;
}

JSAPI_FUNC(control_getText) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  auto self = args.thisv();
  ControlData* pData = ((ControlData*)JS_GetPrivate(self.toObjectOrNull()));
  if (!pData) return JS_TRUE;

  Control* pControl =
      findControl(pData->dwType, (const wchar_t*)NULL, -1, pData->dwX, pData->dwY, pData->dwSizeX, pData->dwSizeY);
  if (!pControl) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  if (pControl->dwType != 4 || !pControl->pFirstText) return JS_TRUE;
  JSAutoRequest r(cx);
  JSObject* pReturnArray = JS_NewArrayObject(cx, 0, NULL);
  args.rval().setObjectOrNull(pReturnArray);

  int nArrayCount = 0;

  for (ControlText* pText = pControl->pFirstText; pText; pText = pText->pNext) {
    if (!pText->wText[0]) continue;

    if (pText->wText[1]) {
      JSObject* pSubArray = JS_NewArrayObject(cx, 0, NULL);

      for (int i = 0; i < 5; i++) {
        if (pText->wText[i]) {
          JS::Value aString = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, pText->wText[i]));
          JS_SetElement(cx, pSubArray, i, &aString);
        }
      }

      JS::Value sub = OBJECT_TO_JSVAL(pSubArray);
      JS_SetElement(cx, pReturnArray, nArrayCount, &sub);
    } else {
      JS::Value aString = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, pText->wText[0]));
      JS_SetElement(cx, pReturnArray, nArrayCount, &aString);
    }

    nArrayCount++;
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getControl) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  int32_t nType = -1, nX = -1, nY = -1, nXSize = -1, nYSize = -1;
  int32_t* argv[] = {&nType, &nX, &nY, &nXSize, &nYSize};
  {
    JSAutoRequest r(cx);
    for (uint32_t i = 0; i < args.length(); i++)
      if (args[i].isInt32()) JS_ValueToECMAInt32(cx, args[i], argv[i]);
  }

  Control* pControl = findControl(nType, (const wchar_t*)NULL, -1, nX, nY, nXSize, nYSize);
  if (!pControl) return JS_TRUE;

  ControlData* data = new ControlData;
  data->dwType = pControl->dwType;
  data->dwX = pControl->dwPosX;
  data->dwY = pControl->dwPosY;
  data->dwSizeX = pControl->dwSizeX;
  data->dwSizeY = pControl->dwSizeY;
  data->pControl = pControl;

  JSObject* control = BuildObject(cx, &control_class, control_funcs, control_props, data);
  if (!control) THROW_ERROR(cx, "Failed to build control!");

  args.rval().setObjectOrNull(control);
  return JS_TRUE;
}

JSAPI_FUNC(my_getControls) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  DWORD dwArrayCount = NULL;

  JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));
  JSAutoRequest r(cx);

  for (Control* pControl = *p_D2WIN_FirstControl; pControl; pControl = pControl->pNext) {
    ControlData* data = new ControlData;
    data->dwType = pControl->dwType;
    data->dwX = pControl->dwPosX;
    data->dwY = pControl->dwPosY;
    data->dwSizeX = pControl->dwSizeX;
    data->dwSizeY = pControl->dwSizeY;
    data->pControl = pControl;

    JSObject* res = BuildObject(cx, &control_class, control_funcs, control_props, data);
    JS::Value a = OBJECT_TO_JSVAL(res);
    JS_SetElement(cx, pReturnArray, dwArrayCount, &a);
    dwArrayCount++;
  }

  args.rval().setObjectOrNull(pReturnArray.get());
  return JS_TRUE;
}
