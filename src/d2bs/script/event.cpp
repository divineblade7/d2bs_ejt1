#include "d2bs/script/event.h"

#include "d2bs/core/Core.h"
#include "d2bs/engine.h"
#include "d2bs/script/ScriptEngine.h"

bool __fastcall LifeEventCallback(Script* script, void* argv, uint) {
  SingleArgHelper* helper = (SingleArgHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("melife")) {
    auto evt = std::make_shared<LifeEvent>();
    evt->owner = script;
    evt->name = "melife";
    evt->life = helper->arg1;

    script->FireEvent(evt);
  }
  return true;
}

void FireLifeEvent(DWORD dwLife) {
  SingleArgHelper helper = {dwLife};
  sScriptEngine->ForEachScript(LifeEventCallback, &helper, 1);
}

bool __fastcall ManaEventCallback(Script* script, void* argv, uint) {
  SingleArgHelper* helper = (SingleArgHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("memana")) {
    auto evt = std::make_shared<ManaEvent>();
    evt->owner = script;
    evt->name = "memana";
    evt->mana = helper->arg1;

    script->FireEvent(evt);
  }
  return true;
}

void FireManaEvent(DWORD dwMana) {
  SingleArgHelper helper = {dwMana};
  sScriptEngine->ForEachScript(ManaEventCallback, &helper, 1);
}

bool __fastcall KeyEventCallback(Script* script, void* argv, uint) {
  KeyEventHelper* helper = (KeyEventHelper*)argv;
  const char* name = (helper->up ? "keyup" : "keydown");
  auto evt = std::make_shared<KeyEvent>();

  if (script->is_running() && script->IsListenerRegistered(name)) {
    evt->owner = script;
    evt->name = name;
    evt->key = helper->key;
    evt->up = helper->up;

    script->FireEvent(evt);
  }

  name = (helper->up ? "keyupblocker" : "keydownblocker");
  if (script->is_running() && script->IsListenerRegistered(name)) {
    evt->owner = script;
    evt->name = name;
    evt->key = helper->key;
    evt->up = helper->up;

    ResetEvent(Vars.eventSignal);
    script->FireEvent(evt);

    if (WaitForSingleObject(Vars.eventSignal, 1000) == WAIT_TIMEOUT) {
      return false;
    }
  }

  return evt->block;
}

bool FireKeyDownUpEvent(WPARAM key, BYTE bUp) {
  KeyEventHelper helper = {bUp, key};
  return sScriptEngine->ForEachScript(KeyEventCallback, &helper, 2);
}

bool __fastcall PlayerAssignCallback(Script* script, void* argv, uint) {
  SingleArgHelper* helper = (SingleArgHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("playerassign")) {
    auto evt = std::make_shared<PlayerAssignEvent>();
    evt->owner = script;
    evt->name = "playerassign";
    evt->unit_id = helper->arg1;

    script->FireEvent(evt);
  }
  return true;
}

void FirePlayerAssignEvent(DWORD dwUnitId) {
  SingleArgHelper helper = {dwUnitId};
  sScriptEngine->ForEachScript(PlayerAssignCallback, &helper, 1);
}

bool __fastcall MouseClickCallback(Script* script, void* argv, uint) {
  QuadArgHelper* helper = (QuadArgHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("mouseclick")) {
    auto evt = std::make_shared<MouseClickEvent>();
    evt->owner = script;
    evt->name = "mouseclick";
    evt->button = helper->arg1;
    evt->x = helper->arg2;
    evt->y = helper->arg3;
    evt->up = helper->arg4;

    script->FireEvent(evt);
  }
  return true;
}

void FireMouseClickEvent(int button, POINT pt, bool bUp) {
  QuadArgHelper helper = {static_cast<DWORD>(button), static_cast<DWORD>(pt.x), static_cast<DWORD>(pt.y),
                          static_cast<DWORD>(bUp)};
  sScriptEngine->ForEachScript(MouseClickCallback, &helper, 4);
}

bool __fastcall MouseMoveCallback(Script* script, void* argv, uint) {
  DoubleArgHelper* helper = (DoubleArgHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("mousemove")) {
    auto evt = std::make_shared<MouseMoveEvent>();
    evt->owner = script;
    evt->name = "mousemove";
    evt->x = helper->arg1;
    evt->y = helper->arg2;

    script->FireEvent(evt);
  }
  return true;
}

void FireMouseMoveEvent(POINT pt) {
  if (pt.x < 1 || pt.y < 1) return;
  DoubleArgHelper helper = {static_cast<DWORD>(pt.x), static_cast<DWORD>(pt.y)};
  sScriptEngine->ForEachScript(MouseMoveCallback, &helper, 2);
}

bool __fastcall BCastEventCallback(Script* script, void* argv, uint argc) {
  BCastEventHelper* helper = (BCastEventHelper*)argv;

  if (script->is_running() && script->IsListenerRegistered("scriptmsg")) {
    auto evt = std::make_shared<BroadcastEvent>();
    evt->owner = script;
    evt->name = "scriptmsg";

    for (uint i = 0; i < argc; ++i) {
      evt->args.push_back(std::make_shared<JSAutoStructuredCloneBuffer>());
      evt->args.back()->write(helper->cx, helper->argv[i]);
    }

    script->FireEvent(evt);
  }
  return true;
}

void FireScriptBroadcastEvent(JSContext* cx, uint argc, jsval* args) {
  BCastEventHelper helper = {cx, args, argc};
  sScriptEngine->ForEachScript(BCastEventCallback, &helper, argc);
}

bool __fastcall ChatEventCallback(Script* script, void* argv, uint) {
  ChatEventHelper* helper = (ChatEventHelper*)argv;
  auto evt = std::make_shared<ChatEvent>();

  if (script->is_running() && script->IsListenerRegistered(helper->name)) {
    evt->owner = script;
    evt->name1 = helper->name;
    evt->nick = helper->nick;
    evt->msg = helper->msg;

    script->FireEvent(evt);
  }

  std::string evtname = helper->name;
  evtname = evtname + "blocker";

  if (script->is_running() && script->IsListenerRegistered(evtname.c_str())) {
    evt->owner = script;
    evt->name = evtname;
    evt->name1 = helper->name;
    evt->nick = helper->nick;
    evt->msg = helper->msg;
    ResetEvent(Vars.eventSignal);
    script->FireEvent(evt);

    if (WaitForSingleObject(Vars.eventSignal, 500) == WAIT_TIMEOUT) return false;
  }

  return evt->block;
}

bool FireChatEvent(const char* lpszNick, const wchar_t* lpszMsg) {
  ChatEventHelper helper = {"chatmsg", lpszNick, lpszMsg};
  return sScriptEngine->ForEachScript(ChatEventCallback, &helper, 2);
}

bool FireChatInputEvent(wchar_t* lpszMsg) {
  ChatEventHelper helper = {"chatinput", "me", lpszMsg};
  return sScriptEngine->ForEachScript(ChatEventCallback, &helper, 2);
}

bool FireWhisperEvent(const char* lpszNick, const wchar_t* lpszMsg) {
  ChatEventHelper helper = {"whispermsg", lpszNick, lpszMsg};
  return sScriptEngine->ForEachScript(ChatEventCallback, &helper, 2);
}

bool __fastcall CopyDataCallback(Script* script, void* argv, uint) {
  CopyDataHelper* helper = (CopyDataHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("copydata")) {
    auto evt = std::make_shared<CopyDataEvent>();
    evt->owner = script;
    evt->name = "copydata";
    evt->mode = helper->mode;
    evt->msg = helper->msg;

    script->FireEvent(evt);
  }
  return true;
}

void FireCopyDataEvent(DWORD dwMode, wchar_t* lpszMsg) {
  CopyDataHelper helper = {dwMode, lpszMsg};
  sScriptEngine->ForEachScript(CopyDataCallback, &helper, 2);
}

bool __fastcall ItemEventCallback(Script* script, void* argv, uint) {
  ItemEventHelper* helper = (ItemEventHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("itemaction")) {
    auto evt = std::make_shared<ItemEvent>();
    evt->owner = script;
    evt->name = "itemaction";
    evt->id = helper->id;
    evt->code = helper->code;
    evt->mode = helper->mode;
    evt->global = helper->global;

    script->FireEvent(evt);
  }
  return true;
}

void FireItemActionEvent(DWORD GID, char* Code, BYTE Mode, bool Global) {
  ItemEventHelper helper = {GID, Code, Mode, Global};
  sScriptEngine->ForEachScript(ItemEventCallback, &helper, 4);
}

bool __fastcall GameActionEventCallback(Script* script, void* argv, uint) {
  GameActionEventHelper* helper = (GameActionEventHelper*)argv;
  if (script->is_running() && script->IsListenerRegistered("gameevent")) {
    auto evt = std::make_shared<GameActionEvent>();
    evt->owner = script;
    evt->name = "gameevent";
    evt->mode = helper->mode;
    evt->param1 = helper->param1;
    evt->param2 = helper->param2;
    evt->name1 = helper->name1;
    evt->name2 = helper->name2;

    script->FireEvent(evt);
  }
  return true;
}

void FireGameActionEvent(BYTE mode, DWORD param1, DWORD param2, char* name1, wchar_t* name2) {
  GameActionEventHelper helper = {mode, param1, param2, name1, name2};
  sScriptEngine->ForEachScript(GameActionEventCallback, &helper, 5);
}

bool __fastcall PacketEventCallback(Script* script, void* argv, uint) {
  PacketEventHelper* helper = (PacketEventHelper*)argv;

  if (script->is_running() && script->IsListenerRegistered(helper->name)) {
    auto evt = std::make_shared<PacketEvent>();
    evt->owner = script;
    evt->name1 = helper->name;
    evt->bytes.resize(helper->dwSize);
    memcpy(evt->bytes.data(), helper->pPacket, helper->dwSize);

    if (GetCurrentThreadId() == evt->owner->thread_id()) {
      script->process_events();
      ExecScriptEvent(evt);
    }
    else {
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
  PacketEventHelper helper = {"gamepacket", pPacket, dwSize};
  return sScriptEngine->ForEachScript(PacketEventCallback, &helper, 3);
}

bool FireGamePacketSentEvent(BYTE* pPacket, DWORD dwSize) {
  PacketEventHelper helper = {"gamepacketsent", pPacket, dwSize};
  return sScriptEngine->ForEachScript(PacketEventCallback, &helper, 3);
}

bool FireRealmPacketEvent(BYTE* pPacket, DWORD dwSize) {
  PacketEventHelper helper = {"realmpacket", pPacket, dwSize};
  return sScriptEngine->ForEachScript(PacketEventCallback, &helper, 3);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, &argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), argc, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 5, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin();
       it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 4, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 0, &rval, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, argv, &rval);
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
  for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 4, argv, &rval);
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
  for (FunctionList::iterator it = functions.begin(); it != functions.end(); it++) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 3, argv, &rval);
    block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_EndRequest(cx);
  for (int j = 0; j < 3; j++) JS_RemoveValueRoot(cx, &argv[j]);

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
    for (FunctionList::iterator it = functions.begin(); it != functions.end(); it++)
      JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2 + 1, argv, &rval);
  } else {
    for (FunctionList::iterator it = owner->functions()[name].begin(); it != owner->functions()[name].end(); it++)
      JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }
}

void DisposeEvent::process() {
  owner->engine()->DisposeScript(owner);
}
