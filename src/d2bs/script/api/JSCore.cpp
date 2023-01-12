#include "d2bs/script/api/JSCore.h"

#include "d2bs/core/Core.h"
#include "d2bs/core/File.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/new_util/localization.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/script/api/JSScript.h"
#include "d2bs/script/event.h"
#include "d2bs/utils/Console.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

#include <ddeml.h>
#include <io.h>
#include <iostream>
#include <utility>
#include <windows.h>
#include <wininet.h>

JSAPI_FUNC(my_stringToEUC) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() == 0 || args[0].isNull()) {
    args.rval().setNull();
    return JS_TRUE;
  }

  JSAutoRequest r(cx);
  if (!JS_ConvertValue(cx, args[0], JSTYPE_STRING, &args[0])) {
    THROW_ERROR(cx, "Converting to string failed");
  }

  const wchar_t* szText = JS_GetStringCharsZ(cx, args[0].toString());
  if (szText == NULL) {
    THROW_ERROR(cx, "Could not get string for value");
  }

  auto euc = d2bs::util::wide_to_utf8(szText);
  args.rval().setString(JS_NewStringCopyZ(cx, euc.c_str()));
  return JS_TRUE;
}

JSAPI_FUNC(my_print) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  for (uint32_t i = 0; i < args.length(); i++) {
    if (!args[i].isNull()) {
      JSAutoRequest r(cx);
      if (!JS_ConvertValue(cx, args[i], JSTYPE_STRING, &args[i])) {
        THROW_ERROR(cx, "Converting to string failed");
      }

      const wchar_t* Text = JS_GetStringCharsZ(cx, args[i].toString());
      if (Text == NULL) {
        THROW_ERROR(cx, "Could not get string for value");
      }

      if (!Text)
        Print(L"undefined");
      else
        Print(L"%s", Text);
    }
  }

  args.rval().setNull();
  return JS_TRUE;
}

JSAPI_FUNC(my_setTimeout) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 2 || !JSVAL_IS_FUNCTION(cx, args[0]) || !args[1].isNumber()) {
    THROW_ERROR(cx, "invalid params passed to setTimeout");
  }

  Script* self = (Script*)JS_GetContextPrivate(cx);
  int freq = JSVAL_TO_INT(args[1]);
  self->RegisterEvent("setTimeout", args[0]);
  auto evt = std::make_shared<TimeoutEvent>(self, "setTimeout", new jsval(args[0]));
  args.rval().setInt32(sScriptEngine->AddDelayedEvent(evt, freq));
  return JS_TRUE;
}

JSAPI_FUNC(my_setInterval) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 2 || !JSVAL_IS_FUNCTION(cx, args[0]) || !args[1].isNumber()) {
    THROW_ERROR(cx, "invalid params passed to setInterval");
  }

  Script* self = (Script*)JS_GetContextPrivate(cx);
  int freq = JSVAL_TO_INT(args[1]);
  self->RegisterEvent("setInterval", args[0]);
  auto evt = std::make_shared<TimeoutEvent>(self, "setInterval", new jsval(args[0]));
  args.rval().setInt32(sScriptEngine->AddDelayedEvent(evt, freq));
  return JS_TRUE;
}

JSAPI_FUNC(my_clearInterval) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1 || !args[0].isNumber()) {
    THROW_ERROR(cx, "invalid params passed to clearInterval");
  }

  sScriptEngine->RemoveDelayedEvent(args[0].toInt32());
  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_delay) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  uint32_t nDelay = 0;
  {
    JSAutoRequest r(cx);
    if (!JS_ConvertArguments(cx, argc, args.array(), "u", &nDelay)) {
      return JS_FALSE;
    }
  }

  if (!nDelay) {
    nDelay = 1;
  }

  Script* script = (Script*)JS_GetContextPrivate(cx);
  DWORD start = GetTickCount();

  int amt = nDelay - (GetTickCount() - start);

  if (nDelay) {        // loop so we can exec events while in delay
    while (amt > 0) {  // had a script deadlock here, make sure were positve with amt
      if (script->is_stopped()) {
        break;
      }

      script->request_interrupt();

      if (JS_GetGCParameter(script->runtime(), JSGC_BYTES) - script->last_gc() > 524288)  // gc every .5 mb
      {
        JS_GC(JS_GetRuntime(cx));
        script->set_last_gc(JS_GetGCParameter(script->runtime(), JSGC_BYTES));
      }

      amt = nDelay - (GetTickCount() - start);
    }

  } else
    JS_ReportWarning(cx, "delay(0) called, argument must be >= 1");

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_load) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);
  if (!script) {
    THROW_ERROR(cx, "Failed to get script object");
  }

  const wchar_t* file = JS_GetStringCharsZ(cx, args[0].toString());
  auto path = (Vars.settings.script_dir / file).make_preferred().wstring();

  if (path.length() > (_MAX_FNAME + _MAX_PATH)) {
    THROW_ERROR(cx, "Filename too long!");
  }

  ScriptType type = script->type();
  if (type == ScriptType::Command) {
    type = (ClientState() == ClientStateInGame ? ScriptType::InGame : ScriptType::OutOfGame);
  }

  JSAutoStructuredCloneBuffer** autoBuffer = new JSAutoStructuredCloneBuffer*[args.length() - 1];
  for (uint32_t i = 1; i < args.length(); i++) {
    autoBuffer[i - 1] = new JSAutoStructuredCloneBuffer;
    autoBuffer[i - 1]->write(cx, args[i]);
  }

  Script* newScript = sScriptEngine->CompileFile(path.c_str(), type, args.length() - 1, autoBuffer);
  if (newScript) {
    newScript->BeginThread(ScriptThread);
  } else {
    Print(L"File \"%s\" not found.", file);
  }

  args.rval().setNull();
  return JS_TRUE;
}

JSAPI_FUNC(my_include) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);
  if (!script) {
    THROW_ERROR(cx, "Failed to get script object");
  }

  const wchar_t* file = JS_GetStringCharsZ(cx, args[0].toString());
  auto path = (Vars.settings.script_dir / "libs" / file).make_preferred().wstring();
  if (path.length() > (_MAX_FNAME + _MAX_PATH)) {
    THROW_ERROR(cx, "Filename too long!");
  }

  if (_waccess(path.c_str(), 0) == 0) {
    args.rval().setBoolean(script->Include(path.c_str()));
  } else {
    args.rval().setBoolean(false);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_stop) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() > 0 && (args[0].isInt32() && args[0].toInt32() == 1) ||
      (args[0].isBoolean() && args[0].toBoolean())) {
    Script* script = (Script*)JS_GetContextPrivate(cx);
    if (script) script->stop();
  } else
    sScriptEngine->StopAll();

  return JS_FALSE;
}

JSAPI_FUNC(my_stacktrace) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

#if DEBUG
  GetStackWalk();
#else
  Log(L"Stackwalk is only enabled in debug builds");
#endif

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_beep) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  int32_t nBeepId = NULL;

  if (args.length() > 0 && args[0].isInt32()) nBeepId = args[0].toInt32();

  MessageBeep(nBeepId);
  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_getTickCount) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setNumber(static_cast<uint32_t>(GetTickCount()));
  return JS_TRUE;
}

JSAPI_FUNC(my_getThreadPriority) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setNumber(reinterpret_cast<uint32_t>(GetCurrentThread()));
  return JS_TRUE;
}

JSAPI_FUNC(my_isIncluded) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  const wchar_t* file = JS_GetStringCharsZ(cx, args[0].toString());
  auto path = (Vars.settings.script_dir / "libs" / file).make_preferred().wstring();

  if (path.length() > (_MAX_FNAME + _MAX_PATH)) {
    THROW_ERROR(cx, "Filename too long!");
  }

  Script* script = (Script*)JS_GetContextPrivate(cx);
  args.rval().setBoolean(script->IsIncluded(path.c_str()));
  return JS_TRUE;
}

JSAPI_FUNC(my_version) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setBoolean(true);

  if (args.length() < 1) {
    args.rval().setString(JS_InternString(cx, D2BS_VERSION));
    return JS_TRUE;
  }

  auto ver = d2bs::util::utf8_to_wide(D2BS_VERSION);
  Print(L"\u00FFc4D2BS\u00FFc1 \u00FFc3%s for Diablo II 1.14d.", ver.c_str());
  return JS_TRUE;
}

JSAPI_FUNC(my_debugLog) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  for (uint32_t i = 0; i < args.length(); i++) {
    if (!args[i].isNull()) {
      {
        JSAutoRequest r(cx);
        if (!JS_ConvertValue(cx, args[i], JSTYPE_STRING, &args[i])) {
          THROW_ERROR(cx, "Converting to string failed");
        }
      }

      const wchar_t* Text = JS_GetStringCharsZ(cx, args[i].toString());
      if (Text == NULL) {
        THROW_ERROR(cx, "Could not get string for value");
      }

      if (!Text)
        Log(L"undefined");
      else {
        Log(L"%s", Text);
      }
    }
  }

  args.rval().setUndefined();
  return JS_TRUE;
}
JSAPI_FUNC(my_copy) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  char* data = JS_EncodeStringToUTF8(cx, args[0].toString());

  HGLOBAL hText;
  char* pText;
  hText = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, strlen(data) + 1);
  pText = (char*)GlobalLock(hText);

  strcpy_s(pText, strlen(data) + 1, data);
  GlobalUnlock(hText);

  OpenClipboard(NULL);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, hText);
  CloseClipboard();
  JS_free(cx, data);

  args.rval().setUndefined();
  return JS_TRUE;
}
JSAPI_FUNC(my_paste) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  OpenClipboard(NULL);
  HANDLE foo = GetClipboardData(CF_TEXT);
  CloseClipboard();
  LPVOID lptstr = GlobalLock(foo);

  args.rval().setString(JS_NewStringCopyZ(cx, static_cast<const char*>(lptstr)));
  return JS_TRUE;
}
JSAPI_FUNC(my_sendCopyData) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setBoolean(false);
  if (args.length() < 4) {
    return JS_TRUE;
  }

  const wchar_t *windowClassName = NULL, *windowName = NULL;
  char* data = NULL;
  int32_t nModeId = NULL;
  HWND hWnd = NULL;

  {
    JSAutoRequest r(cx);
    if (args[0].isString() && !args[0].isNull()) {
      windowClassName = JS_GetStringCharsZ(cx, args[0].toString());
    }

    if (!args[1].isNull()) {
      if (args[1].isNumber()) {
        JS_ValueToECMAUint32(cx, args[1], (uint32_t*)&hWnd);
      } else if (args[1].isString()) {
        windowName = JS_GetStringCharsZ(cx, args[1].toString());
      }
    }

    if (args[2].isNumber() && !args[2].isNull()) {
      JS_ValueToECMAUint32(cx, args[2], (uint32_t*)&nModeId);
    }

    if (args[3].isString() && !args[3].isNull()) {
      data = JS_EncodeStringToUTF8(cx, args[3].toString());
    }
  }

  if (hWnd == NULL && windowName != NULL) {
    hWnd = FindWindowW(windowClassName, windowName);
    if (!hWnd) {
      JS_free(cx, data);
      return JS_TRUE;
    }
  }

  // if data is NULL, strlen crashes
  if (data) {
    COPYDATASTRUCT aCopy = {static_cast<ULONG_PTR>(nModeId), strlen(data) + 1, data};
    args.rval().setInt32(SendMessage(hWnd, WM_COPYDATA, (WPARAM)D2GFX_GetHwnd(), (LPARAM)&aCopy));
    JS_free(cx, data);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_sendDDE) {
  THROW_ERROR(cx, "sendDDE has been deprecated!");
}

JSAPI_FUNC(my_keystate) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 1 || !args[0].isInt32()) {
    args.rval().setBoolean(false);
    return JS_TRUE;
  }

  args.rval().setBoolean(!!GetAsyncKeyState(args[0].toInt32()));
  return JS_TRUE;
}

JSAPI_FUNC(my_addEventListener) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args[0].isString() && JSVAL_IS_FUNCTION(cx, args[1])) {
    char* evtName = JS_EncodeStringToUTF8(cx, args[0].toString());
    if (evtName && strlen(evtName)) {
      Script* self = (Script*)JS_GetContextPrivate(cx);
      self->RegisterEvent(evtName, args[1]);
    } else
      THROW_ERROR(cx, "Event name is invalid!");
    JS_free(cx, evtName);
  }

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_removeEventListener) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args[0].isString() && JSVAL_IS_FUNCTION(cx, args[1])) {
    char* evtName = JS_EncodeStringToUTF8(cx, args[0].toString());
    if (evtName && strlen(evtName)) {
      Script* self = (Script*)JS_GetContextPrivate(cx);
      self->UnregisterEvent(evtName, JS_ARGV(cx, vp)[1]);
    } else
      THROW_ERROR(cx, "Event name is invalid!");
    JS_free(cx, evtName);
  }

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_clearEvent) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args[0].isString()) {
    Script* self = (Script*)JS_GetContextPrivate(cx);
    char* evt = JS_EncodeStringToUTF8(cx, args[0].toString());
    self->ClearEvent(evt);
    JS_free(cx, evt);
  }

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_clearAllEvents) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* self = (Script*)JS_GetContextPrivate(cx);
  self->ClearAllEvents();

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_js_strict) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() == 0) {
    args.rval().setBoolean(((JS_GetOptions(cx) & JSOPTION_STRICT) == JSOPTION_STRICT));
    return JS_TRUE;
  }

  if (args.length() == 1) {
    bool bFlag = ((JS_GetOptions(cx) & JSOPTION_STRICT) == JSOPTION_STRICT);
    if (args[0].toBoolean()) {
      if (!bFlag) JS_ToggleOptions(cx, JSOPTION_STRICT);
    } else {
      if (bFlag) JS_ToggleOptions(cx, JSOPTION_STRICT);
    }
  }

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_scriptBroadcast) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  if (args.length() < 1) {
    THROW_ERROR(cx, "You must specify something to broadcast");
  }

  FireScriptBroadcastEvent(cx, args.length(), args.array());
  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_showConsole) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  sConsole->Show();
  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_hideConsole) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  sConsole->Hide();
  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_handler) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setInt32(reinterpret_cast<uint32_t>(Vars.hHandle));
  return JS_TRUE;
}

JSAPI_FUNC(my_loadMpq) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  const wchar_t* path = JS_GetStringCharsZ(cx, args[0].toString());

  if (isValidPath(path)) {
    LoadMPQ(path);
  }

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_sendPacket) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS_SET_RVAL(cx, vp, JSVAL_NULL);

  if (!Vars.settings.bEnableUnsupported) {
    THROW_WARNING(cx, vp, "sendPacket requires EnableUnsupported = true in d2bs.ini");
  }

  BYTE* aPacket;
  uint32_t len = 0;
  JSAutoRequest r(cx);

  if (args[0].isObject()) {
    JSObject* obj;
    JS_ValueToObject(cx, args[0], &obj);

    if (!JS_IsArrayBufferObject(obj)) {
      THROW_WARNING(cx, vp, "invalid ArrayBuffer parameter");
    }

    len = JS_GetArrayBufferByteLength(obj);
    aPacket = new BYTE[len];
    memcpy(aPacket, JS_GetArrayBufferData(obj), len);
  } else {
    if (args.length() % 2 != 0) {
      THROW_WARNING(cx, vp, "invalid packet format");
    }

    aPacket = new BYTE[2 * argc];
    uint32_t size = 0;

    for (uint32_t i = 0; i < argc; i += 2, len += size) {
      JS_ValueToECMAUint32(cx, args[i], &size);
      JS_ValueToECMAUint32(cx, args[i + 1], (uint32_t*)&aPacket[len]);
    }
  }

  D2NET_SendPacket(len, 1, aPacket);
  delete[] aPacket;
  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_getPacket) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS_SET_RVAL(cx, vp, JSVAL_NULL);

  if (!Vars.settings.bEnableUnsupported) {
    THROW_WARNING(cx, vp, "getPacket requires EnableUnsupported = true in d2bs.ini");
  }

  JSAutoRequest r(cx);
  BYTE* aPacket;
  uint32_t len = 0;

  if (args[0].isObject()) {
    JSObject* obj = args[0].toObjectOrNull();
    if (!JS_IsArrayBufferObject(obj)) {
      THROW_WARNING(cx, vp, "invalid ArrayBuffer parameter");
    }

    len = JS_GetArrayBufferByteLength(obj);
    aPacket = new BYTE[len];
    memcpy(aPacket, JS_GetArrayBufferData(obj), len);
  } else {
    if (argc % 2 != 0) {
      THROW_WARNING(cx, vp, "invalid packet format");
    }

    aPacket = new BYTE[2 * argc];
    uint32_t size = 0;

    for (uint32_t i = 0; i < argc; i += 2, len += size) {
      JS_ValueToECMAUint32(cx, args[i], &size);
      JS_ValueToECMAUint32(cx, args[i + 1], (uint32_t*)&aPacket[len]);
    }
  }

  D2NET_ReceivePacket(aPacket, len);
  delete[] aPacket;
  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(my_getIP) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  HINTERNET hInternet, hFile;
  DWORD rSize;
  char buffer[32];

  hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  hFile = InternetOpenUrl(hInternet, "http://ipv4bot.whatismyipaddress.com", NULL, 0, INTERNET_FLAG_RELOAD, 0);
  InternetReadFile(hFile, &buffer, sizeof(buffer), &rSize);
  buffer[std::min(rSize, DWORD(31))] = '\0';
  InternetCloseHandle(hFile);
  InternetCloseHandle(hInternet);

  JSAutoRequest r(cx);
  args.rval().setString(JS_NewStringCopyZ(cx, buffer));
  return JS_TRUE;
}

JSAPI_FUNC(my_sendClick) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  uint32_t x = 0;
  uint32_t y = 0;
  {
    JSAutoRequest r(cx);
    if (!JS_ConvertArguments(cx, argc, args.array(), "uu", &x, &y)) {
      return JS_FALSE;
    }
  }

  Sleep(100);
  SendMouseClick(x, y, 0);
  Sleep(100);
  SendMouseClick(x, y, 1);
  Sleep(100);

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(my_sendKey) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  uint32_t key;
  {
    JSAutoRequest r(cx);
    if (!JS_ConvertArguments(cx, argc, args.array(), "u", &key)) {
      return JS_FALSE;
    }
  }

  BOOL prompt = sConsole->IsEnabled();
  if (prompt) {
    sConsole->HidePrompt();
  }

  Sleep(100);
  SendKeyPress(WM_KEYDOWN, key, 0);
  Sleep(100);
  SendKeyPress(WM_KEYUP, key, 0);
  Sleep(100);

  if (prompt) {
    sConsole->ShowPrompt();
  }

  args.rval().setUndefined();
  return JS_TRUE;
}
