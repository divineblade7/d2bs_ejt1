#include "d2bs/script/event.h"

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

    if (GetCurrentThreadId() == evt->owner->thread_id())
      ExecScriptEvent(evt, false);
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
