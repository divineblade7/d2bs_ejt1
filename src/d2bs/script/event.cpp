#include "d2bs/script/event.h"

#include "d2bs/core/Core.h"
#include "d2bs/engine.h"
#include "d2bs/script/ScriptEngine.h"

void FireLifeEvent(DWORD dwLife) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("melife")) {
      auto evt = std::make_shared<LifeEvent>();
      evt->owner = script;
      evt->name = "melife";
      evt->life = dwLife;

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireManaEvent(DWORD dwMana) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("memana")) {
      auto evt = std::make_shared<ManaEvent>();
      evt->owner = script;
      evt->name = "memana";
      evt->mana = dwMana;

      script->FireEvent(evt);
    }
    return true;
  });
}

bool FireKeyDownUpEvent(WPARAM key, BYTE bUp) {
  return sScriptEngine->for_each([&](Script* script) {
    const char* name = (bUp ? "keyup" : "keydown");
    auto evt = std::make_shared<KeyEvent>();
    evt->owner = script;
    evt->name = name;
    evt->key = key;
    evt->up = bUp;

    if (script->is_running() && script->IsListenerRegistered(name)) {
      script->FireEvent(evt);
    }

    name = (bUp ? "keyupblocker" : "keydownblocker");
    if (script->is_running() && script->IsListenerRegistered(name)) {
      evt->name = name;

      ResetEvent(Vars.eventSignal);
      script->FireEvent(evt);

      if (WaitForSingleObject(Vars.eventSignal, 1000) == WAIT_TIMEOUT) {
        return false;
      }
    }

    return evt->block;
  });
}

void FirePlayerAssignEvent(DWORD dwUnitId) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("playerassign")) {
      auto evt = std::make_shared<PlayerAssignEvent>();
      evt->owner = script;
      evt->name = "playerassign";
      evt->unit_id = dwUnitId;

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireMouseClickEvent(int button, POINT pt, bool bUp) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("mouseclick")) {
      auto evt = std::make_shared<MouseClickEvent>();
      evt->owner = script;
      evt->name = "mouseclick";
      evt->button = button;
      evt->x = static_cast<DWORD>(pt.x);
      evt->y = static_cast<DWORD>(pt.y);
      evt->up = bUp;

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireMouseMoveEvent(POINT pt) {
  if (pt.x < 1 || pt.y < 1) {
    return;
  }

  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("mousemove")) {
      auto evt = std::make_shared<MouseMoveEvent>();
      evt->owner = script;
      evt->name = "mousemove";
      evt->x = static_cast<DWORD>(pt.x);
      evt->y = static_cast<DWORD>(pt.y);

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireScriptBroadcastEvent(JSContext* cx, uint argc, jsval* args) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("scriptmsg")) {
      auto evt = std::make_shared<BroadcastEvent>();
      evt->owner = script;
      evt->name = "scriptmsg";

      for (uint i = 0; i < argc; ++i) {
        evt->args.push_back(std::make_shared<JSAutoStructuredCloneBuffer>());
        evt->args.back()->write(cx, args[i]);
      }

      script->FireEvent(evt);
    }
    return true;
  });
}

bool ChatEventCallback(Script* script, const char* name, const char* nick, const wchar_t* msg) {
  auto evt = std::make_shared<ChatEvent>();

  if (script->is_running() && script->IsListenerRegistered(name)) {
    evt->owner = script;
    evt->name1 = name;
    evt->nick = nick;
    evt->msg = msg;

    script->FireEvent(evt);
  }

  std::string evtname = name;
  evtname = evtname + "blocker";

  if (script->is_running() && script->IsListenerRegistered(evtname.c_str())) {
    evt->owner = script;
    evt->name = evtname;
    evt->name1 = name;
    evt->nick = nick;
    evt->msg = msg;
    ResetEvent(Vars.eventSignal);
    script->FireEvent(evt);

    if (WaitForSingleObject(Vars.eventSignal, 500) == WAIT_TIMEOUT) return false;
  }

  return evt->block;
}

bool FireChatEvent(const char* lpszNick, const wchar_t* lpszMsg) {
  return sScriptEngine->for_each(ChatEventCallback, "chatmsg", lpszNick, lpszMsg);
}

bool FireChatInputEvent(wchar_t* lpszMsg) {
  return sScriptEngine->for_each(ChatEventCallback, "chatinput", "me", lpszMsg);
}

bool FireWhisperEvent(const char* lpszNick, const wchar_t* lpszMsg) {
  return sScriptEngine->for_each(ChatEventCallback, "whispermsg", lpszNick, lpszMsg);
}

void FireCopyDataEvent(DWORD dwMode, wchar_t* lpszMsg) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("copydata")) {
      auto evt = std::make_shared<CopyDataEvent>();
      evt->owner = script;
      evt->name = "copydata";
      evt->mode = dwMode;
      evt->msg = lpszMsg;

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireItemActionEvent(DWORD GID, char* Code, BYTE Mode, bool Global) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("itemaction")) {
      auto evt = std::make_shared<ItemEvent>();
      evt->owner = script;
      evt->name = "itemaction";
      evt->id = GID;
      evt->code = Code;
      evt->mode = Mode;
      evt->global = Global;

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireGameActionEvent(BYTE mode, DWORD param1, DWORD param2, char* name1, wchar_t* name2) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("gameevent")) {
      auto evt = std::make_shared<GameActionEvent>();
      evt->owner = script;
      evt->name = "gameevent";
      evt->mode = mode;
      evt->param1 = param1;
      evt->param2 = param2;
      evt->name1 = name1;
      evt->name2 = name2;

      script->FireEvent(evt);
    }
    return true;
  });
}

bool PacketEventCallback(Script* script, const char* name, BYTE* pPacket, DWORD dwSize) {
  if (script->is_running() && script->IsListenerRegistered(name)) {
    auto evt = std::make_shared<PacketEvent>();
    evt->owner = script;
    evt->name1 = name;
    evt->bytes.resize(dwSize);
    memcpy(evt->bytes.data(), pPacket, dwSize);

    if (GetCurrentThreadId() == evt->owner->thread_id()) {
      script->process_events();
    } else {
      ResetEvent(Vars.eventSignal);
      script->FireEvent(evt);
      static DWORD result;
      ReleaseGameLock();
      result = WaitForSingleObject(Vars.eventSignal, 500);
      TakeGameLock();

      if (result == WAIT_TIMEOUT) return false;
    }

    return evt->block;
  }

  return false;
}

bool FireGamePacketEvent(BYTE* pPacket, DWORD dwSize) {
  return sScriptEngine->for_each(PacketEventCallback, "gamepacket", pPacket, dwSize);
}

bool FireGamePacketSentEvent(BYTE* pPacket, DWORD dwSize) {
  return sScriptEngine->for_each(PacketEventCallback, "gamepacketsent", pPacket, dwSize);
}

bool FireRealmPacketEvent(BYTE* pPacket, DWORD dwSize) {
  return sScriptEngine->for_each(PacketEventCallback, "realmpacket", pPacket, dwSize);
}

void ReleaseGameLock(void) {
  if (Vars.bGameLoopEntered && Vars.dwGameThreadId == GetCurrentThreadId())
    LeaveCriticalSection(&Vars.cGameLoopSection);
}

void TakeGameLock(void) {
  if (Vars.bGameLoopEntered && Vars.dwGameThreadId == GetCurrentThreadId())
    EnterCriticalSection(&Vars.cGameLoopSection);
}

void CopyDataEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(mode);
  argv[1] = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, msg.c_str()));

  for (int j = 0; j < 2; j++) JS_AddValueRoot(cx, &argv[j]);

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }
}

void CommandEvent::process() {
  auto cx = owner->context();

  // copy the command
  std::wstring cmd = command;
  cmd.append(L"try{ ");
  cmd.append(cmd);
  cmd.append(L" } catch (error){print(error)}");

  JS_BeginRequest(cx);
  jsval rval;

  if (JS_EvaluateUCScript(cx, JS_GetGlobalObject(cx), cmd.data(), cmd.length(), "Command Line", 0, &rval)) {
    if (!JSVAL_IS_NULL(rval) && !JSVAL_IS_VOID(rval)) {
      JS_ConvertValue(cx, rval, JSTYPE_STRING, &rval);
      const wchar_t* text = JS_GetStringCharsZ(cx, JS_ValueToString(cx, rval));
      Print(L"%s", text);
    }
  }
  JS_EndRequest(cx);
}

void ChatEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = (STRING_TO_JSVAL(JS_NewStringCopyZ(cx, nick.c_str())));
  argv[1] = (STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, msg.c_str())));

  for (int j = 0; j < 2; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  if (name == "chatmsgblocker" || name == "chatinputblocker" || name == "whispermsgblocker") {
    SetEvent(Vars.eventSignal);
  }
}

void PacketEvent::process() {
  auto cx = owner->context();

  BYTE* help = bytes.data();
  DWORD size = bytes.size();

  //  DWORD* argc = (DWORD*)1;
  JS_BeginRequest(cx);

  JSObject* arr = JS_NewUint8Array(cx, size);
  // JSObject* arr = JS_NewArrayObject(cx, 0, NULL);

  JS_AddRoot(cx, &arr);
  for (uint i = 0; i < size; i++) {
    jsval jsarr = UINT_TO_JSVAL(help[i]);
    JS_SetElement(cx, arr, i, &jsarr);
  }
  jsval argv = OBJECT_TO_JSVAL(arr);
  // evt->argv[0]->read(cx, &argv[0]);
  // JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, &argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_RemoveRoot(cx, &arr);
  SetEvent(Vars.eventSignal);
  JS_EndRequest(cx);
}

void BroadcastEvent::process() {
  auto cx = owner->context();

  JS_BeginRequest(cx);
  auto argc = args.size();
  jsval* argv = new jsval[args.size()];
  for (uint i = 0; i < argc; i++) {
    args[i]->read(cx, &argv[i]);
  }

  for (uint j = 0; j < argc; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), argc, argv, &rval);
  }
  JS_EndRequest(cx);

  for (uint j = 0; j < argc; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  for (uint i = 0; i < argc; i++) {
    args[i]->clear();
  }
}

void GameActionEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[5];
  JS_BeginRequest(cx);

  argv[0] = JS_NumberValue(mode);
  argv[1] = JS_NumberValue(param1);
  argv[2] = JS_NumberValue(param2);
  argv[3] = (STRING_TO_JSVAL(JS_NewStringCopyZ(cx, name1.c_str())));
  argv[4] = (STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, name2.c_str())));

  for (int j = 0; j < 5; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 5, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 5; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }
}

void KeyEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(key);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);

  if (name == "keydownblocker") {
    SetEvent(Vars.eventSignal);
  }
}

void ItemEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[4];
  JS_BeginRequest(cx);

  argv[0] = JS_NumberValue(id);
  argv[1] = JS_NumberValue(mode);
  argv[2] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, code.c_str()));
  argv[3] = BOOLEAN_TO_JSVAL(global);
  for (int j = 0; j < 4; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 4, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_EndRequest(cx);

  for (int j = 0; j < 4; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }
}

void TimeoutEvent::process() {
  auto cx = owner->context();

  JS_BeginRequest(cx);
  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 0, &rval, &rval);
  }
  JS_EndRequest(cx);

  if (name == "setTimeout") {
    owner->engine()->RemoveDelayedEvent(key);
  }
}

void LifeEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(life);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);
}

void ManaEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(mana);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);
}

void PlayerAssignEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(unit_id);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);
}

void MouseClickEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[4];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(button);
  argv[1] = JS_NumberValue(x);
  argv[2] = JS_NumberValue(y);
  argv[3] = JS_NumberValue(up);

  for (uint j = 0; j < 4; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner->functions()[name]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 4, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 4; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }
}

void ScreenHookClickEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[3];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(button);
  argv[1] = JS_NumberValue(x);
  argv[2] = JS_NumberValue(y);
  for (int j = 0; j < 3; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  // diffrent function source for hooks
  for (const auto& fn : functions) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 3, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_EndRequest(cx);
  for (int j = 0; j < 3; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  SetEvent(Vars.eventSignal);
}

void MouseMoveEvent::process() {
  auto cx = owner->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(x);
  argv[1] = JS_NumberValue(y);

  for (int j = 0; j < 2; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  if (name == "ScreenHookHover") {
    for (const auto& fn : functions) {
      JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2 + 1, argv, &rval);
    }
  } else {
    for (const auto& fn : owner->functions()[name]) {
      JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2, argv, &rval);
    }
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }
}

void DisposeEvent::process() {
  owner->engine()->DisposeScript(owner);
}
