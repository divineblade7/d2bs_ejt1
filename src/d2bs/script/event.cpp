#include "d2bs/script/event.h"

#include "d2bs/core/Core.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/variables.h"

void FireLifeEvent(DWORD dwLife) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("melife")) {
      auto evt = std::make_shared<LifeEvent>(script, dwLife);

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireManaEvent(DWORD dwMana) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("memana")) {
      auto evt = std::make_shared<ManaEvent>(script, dwMana);

      script->FireEvent(evt);
    }
    return true;
  });
}

bool FireKeyDownUpEvent(WPARAM key, BYTE bUp) {
  return sScriptEngine->for_each([&](Script* script) {
    const char* name = (bUp ? "keyup" : "keydown");
    auto evt = std::make_shared<KeyEvent>(script, key, bUp);

    if (script->is_running() && script->IsListenerRegistered(name)) {
      script->FireEvent(evt);
    }

    name = (bUp ? "keyupblocker" : "keydownblocker");
    auto evt_block = std::make_shared<KeyEvent>(script, key, bUp);
    if (script->is_running() && script->IsListenerRegistered(name)) {
      script->FireEvent(evt_block);
      script->request_interrupt();
    }

    evt_block->wait();
    return evt_block->block();
  });
}

void FirePlayerAssignEvent(DWORD dwUnitId) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("playerassign")) {
      auto evt = std::make_shared<PlayerAssignEvent>(script, dwUnitId);

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireMouseClickEvent(int button, POINT pt, bool bUp) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("mouseclick")) {
      auto evt = std::make_shared<MouseClickEvent>(script, button, pt.x, pt.y, bUp);

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
      auto evt = std::make_shared<MouseMoveEvent>(script, pt.x, pt.y);

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireScriptBroadcastEvent(JSContext* cx, uint32_t argc, jsval* args) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("scriptmsg")) {
      std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> arg;

      for (uint32_t i = 0; i < argc; ++i) {
        arg.push_back(std::make_shared<JSAutoStructuredCloneBuffer>());
        arg.back()->write(cx, args[i]);
      }

      auto evt = std::make_shared<BroadcastEvent>(script, arg);

      script->FireEvent(evt);
    }
    return true;
  });
}

bool ChatEventCallback(Script* script, const char* name, const char* nick, const wchar_t* msg) {
  if (script->is_running() && script->IsListenerRegistered(name)) {
    auto evt = std::make_shared<ChatEvent>(script, name, name, nick, msg);
    script->FireEvent(evt);
  }

  std::string evtname = name;
  evtname = evtname + "blocker";
  if (script->is_running() && script->IsListenerRegistered(evtname.c_str())) {
    auto evt_block = std::make_shared<ChatEvent>(script, evtname.c_str(), name, nick, msg);
    script->FireEvent(evt_block);
    script->request_interrupt();
    evt_block->wait();
    return evt_block->block();
  }

  return true;
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
      auto evt = std::make_shared<CopyDataEvent>(script, dwMode, lpszMsg);
      script->FireEvent(evt);
    }
    return true;
  });
}

void FireItemActionEvent(DWORD GID, char* Code, BYTE Mode, bool Global) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("itemaction")) {
      auto evt = std::make_shared<ItemEvent>(script, GID, Code, Mode, Global);

      script->FireEvent(evt);
    }
    return true;
  });
}

void FireGameActionEvent(BYTE mode, DWORD param1, DWORD param2, char* name1, wchar_t* name2) {
  sScriptEngine->for_each([&](Script* script) {
    if (script->is_running() && script->IsListenerRegistered("gameevent")) {
      auto evt = std::make_shared<GameActionEvent>(script, mode, param1, param2, name1, name2);

      script->FireEvent(evt);
    }
    return true;
  });
}

bool PacketEventCallback(Script* script, const char* name, BYTE* pPacket, DWORD dwSize) {
  if (script->is_running() && script->IsListenerRegistered(name)) {
    std::vector<uint8_t> bytes(dwSize);
    memcpy(bytes.data(), pPacket, dwSize);
    auto evt = std::make_shared<PacketEvent>(script, name, bytes);

    script->FireEvent(evt);
    script->request_interrupt();
    evt->wait();
    return evt->block();
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

CopyDataEvent::CopyDataEvent(Script* owner, DWORD mode_, std::wstring msg_)
    : Event(owner, "copydata"), mode(mode_), msg(msg_) {}

void CopyDataEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(mode);
  argv[1] = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, msg.c_str()));

  for (int j = 0; j < 2; j++) JS_AddValueRoot(cx, &argv[j]);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

CommandEvent::CommandEvent(Script* owner, std::wstring command_) : Event(owner, "Command"), command(command_) {}

void CommandEvent::process() {
  auto cx = owner_->context();

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

  is_processed_ = true;
  is_processed_.notify_all();
}

ChatEvent::ChatEvent(Script* owner, const char* name, std::string name1_, std::string nick_, std::wstring msg_)
    : Event(owner, name), name1(name1_), nick(nick_), msg(msg_) {}

void ChatEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = (STRING_TO_JSVAL(JS_NewStringCopyZ(cx, nick.c_str())));
  argv[1] = (STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, msg.c_str())));

  for (int j = 0; j < 2; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

PacketEvent::PacketEvent(Script* owner, std::string name1_, std::vector<uint8_t> bytes_)
    : Event(owner, name1_), name1(name1_), bytes(bytes_) {}

void PacketEvent::process() {
  auto cx = owner_->context();

  BYTE* help = bytes.data();
  DWORD size = bytes.size();

  JS_BeginRequest(cx);

  JS::RootedObject arr(cx,  JS_NewUint8Array(cx, size));

  for (uint32_t i = 0; i < size; i++) {
    jsval jsarr = UINT_TO_JSVAL(help[i]);
    JS_SetElement(cx, arr, i, &jsarr);
  }
  jsval argv = OBJECT_TO_JSVAL(arr);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, &argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_EndRequest(cx);

  is_processed_ = true;
  is_processed_.notify_all();
}

BroadcastEvent::BroadcastEvent(Script* owner, std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> args_)
    : Event(owner, "scriptmsg"), args(args_) {}

void BroadcastEvent::process() {
  auto cx = owner_->context();

  JSAutoRequest r(cx);
  auto argc = args.size();
  jsval* argv = new jsval[args.size()];
  for (uint32_t i = 0; i < argc; i++) {
    args[i]->read(cx, &argv[i]);
  }

  for (uint32_t j = 0; j < argc; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), argc, argv, &rval);
  }

  for (uint32_t j = 0; j < argc; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  for (uint32_t i = 0; i < argc; i++) {
    args[i]->clear();
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

GameActionEvent::GameActionEvent(Script* owner, BYTE mode_, DWORD param1_, DWORD param2_, std::string name1_,
                                 std::wstring name2_)
    : Event(owner, "gameevent"), mode(mode_), param1(param1_), param2(param2_), name1(name1_), name2(name2_) {}

void GameActionEvent::process() {
  auto cx = owner_->context();

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
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 5, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 5; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

KeyEvent::KeyEvent(Script* owner, WPARAM key_, BYTE bUp)
    : Event(owner, bUp ? "keyup" : "keydown"), key(key_), up(bUp) {}

void KeyEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(key);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);

  is_processed_ = true;
  is_processed_.notify_all();
}

ItemEvent::ItemEvent(Script* owner, DWORD id_, std::string code_, WORD mode_, bool global_)
    : Event(owner, "itemaction"), id(id_), code(code_), mode(mode_), global(global_) {}

void ItemEvent::process() {
  auto cx = owner_->context();

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
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 4, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_EndRequest(cx);

  for (int j = 0; j < 4; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

TimeoutEvent::TimeoutEvent(Script* owner, const char* name, jsval* val_) : Event(owner, name), val(val_) {}

void TimeoutEvent::process() {
  auto cx = owner_->context();

  JS_BeginRequest(cx);
  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 0, &rval, &rval);
  }
  JS_EndRequest(cx);

  if (name_ == "setTimeout") {
    sScriptEngine->RemoveDelayedEvent(key);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

LifeEvent::LifeEvent(Script* owner, DWORD life_) : Event(owner, "melife"), life(life_) {}

void LifeEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(life);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);

  is_processed_ = true;
  is_processed_.notify_all();
}

ManaEvent::ManaEvent(Script* owner, DWORD mana_) : Event(owner, "memana"), mana(mana_) {}

void ManaEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(mana);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);

  is_processed_ = true;
  is_processed_.notify_all();
}

PlayerAssignEvent::PlayerAssignEvent(Script* owner, DWORD unitid) : Event(owner, "playerassign"), unit_id(unitid) {}

void PlayerAssignEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(unit_id);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);

  is_processed_ = true;
  is_processed_.notify_all();
}

MouseClickEvent::MouseClickEvent(Script* owner, DWORD button_, DWORD x_, DWORD y_, DWORD up_)
    : Event(owner, "mouseclick"), button(button_), x(x_), y(y_), up(up_) {}

void MouseClickEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[4];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(button);
  argv[1] = JS_NumberValue(x);
  argv[2] = JS_NumberValue(y);
  argv[3] = JS_NumberValue(up);

  for (uint32_t j = 0; j < 4; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 4, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 4; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

ScreenHookClickEvent::ScreenHookClickEvent(Script* owner, FunctionList funcs, int button_, LONG x_, LONG y_)
    : Event(owner, "ScreenHookClick"), functions(funcs), button(button_), x(x_), y(y_) {}

void ScreenHookClickEvent::process() {
  auto cx = owner_->context();

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
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }

  JS_EndRequest(cx);
  for (int j = 0; j < 3; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

MouseMoveEvent::MouseMoveEvent(Script* owner, DWORD x_, DWORD y_) : Event(owner, "mousemove"), x(x_), y(y_) {}

void MouseMoveEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(x);
  argv[1] = JS_NumberValue(y);

  for (int j = 0; j < 2; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}

DisposeEvent::DisposeEvent(Script* owner) : Event(owner, "DisposeMe") {}

void DisposeEvent::process() {
  sScriptEngine->DisposeScript(owner_);
  is_processed_ = true;
  is_processed_.notify_all();
}

Event::Event(Script* owner, std::string name) : owner_(owner), name_(name) {}

void Event::wait() {
  is_processed_.wait(true);
}

void Event::notify_all() {
  is_processed_.notify_all();
}

KeyBlockEvent::KeyBlockEvent(Script* owner, WPARAM key_, BYTE bUp)
    : Event(owner, bUp ? "keyupblocker" : "keydownblocker"), key(key_), up(bUp) {}

void KeyBlockEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[1];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(key);
  JS_AddValueRoot(cx, &argv[0]);

  jsval rval;
  for (const auto& fn : owner_->functions()[name_]) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 1, argv, &rval);
    block_ |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
  }
  JS_EndRequest(cx);
  JS_RemoveValueRoot(cx, &argv[0]);

  is_processed_ = true;
  is_processed_.notify_all();
}

ScreenHookHoverEvent::ScreenHookHoverEvent(Script* owner, FunctionList funcs, LONG x_, LONG y_)
    : Event(owner, "ScreenHookHover"), functions(funcs), x(x_), y(y_) {}

void ScreenHookHoverEvent::process() {
  auto cx = owner_->context();

  jsval* argv = new jsval[2];
  JS_BeginRequest(cx);
  argv[0] = JS_NumberValue(x);
  argv[1] = JS_NumberValue(y);

  for (int j = 0; j < 2; j++) {
    JS_AddValueRoot(cx, &argv[j]);
  }

  jsval rval;
  for (const auto& fn : functions) {
    JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *fn->value(), 2 + 1, argv, &rval);
  }
  JS_EndRequest(cx);
  for (int j = 0; j < 2; j++) {
    JS_RemoveValueRoot(cx, &argv[j]);
  }

  is_processed_ = true;
  is_processed_.notify_all();
}
