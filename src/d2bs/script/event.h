#pragma once

#include "d2bs/script/AutoRoot.h"

#include <Windows.h>
#include <list>
#include <map>

class Script;

typedef std::list<AutoRoot*> FunctionList;
typedef std::map<std::string, FunctionList> FunctionMap;

struct Event {
  Script* owner = nullptr;
  JSObject* object = nullptr;
  FunctionList functions;
  std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> args;
  std::string name;
  bool block = false;
};

struct CopyDataEvent : Event {
  DWORD mode;
  std::wstring msg;
};

struct CommandEvent : Event {
  std::wstring command;
};

struct ChatEvent : Event {
  std::string name1, nick;
  std::wstring msg;
};

struct PacketEvent : Event {
  std::string name1;
  std::vector<uint8_t> bytes;
};

struct BroadcastEvent : Event {};

struct GameActionEvent : Event {
  BYTE mode;
  DWORD param1, param2;
  std::string name1;
  std::wstring name2;
};

struct KeyEvent : Event {
  BOOL up;
  WPARAM key;
};

struct ItemEvent : Event {
  DWORD id;
  std::string code;
  WORD mode;
  bool global;
};

struct TimeoutEvent : Event {
  int key;
  HANDLE handle;
  jsval* val;
};

struct LifeEvent : Event {
  DWORD life;
};

struct ManaEvent : Event {
  DWORD mana;
};

struct PlayerAssignEvent : Event {
  DWORD unit_id;
};

struct MouseClickEvent : Event {
  DWORD button;
  DWORD x;
  DWORD y;
  DWORD up;
};

struct ScreenHookClickEvent : Event {
  int button;
  LONG x;
  LONG y;
};

struct MouseMoveEvent : Event {
  DWORD x;
  DWORD y;
};

bool FireChatEvent(const char* lpszNick, const wchar_t* lpszMsg);
bool FireChatInputEvent(wchar_t* lpszMsg);
void FireLifeEvent(DWORD dwLife);
void FireManaEvent(DWORD dwMana);
void FireCopyDataEvent(DWORD dwMode, wchar_t* lpszMsg);
void FireGameActionEvent(BYTE mode, DWORD param1, DWORD param2, char* name1, wchar_t* name2);
bool FireWhisperEvent(const char* lpszNick, const wchar_t* lpszMsg);
bool FireKeyDownUpEvent(WPARAM bByte, BYTE bUp);
void FirePlayerAssignEvent(DWORD dwUnitId);
void FireMouseClickEvent(int button, POINT pt, bool bUp);
void FireMouseMoveEvent(POINT pt);
void FireScriptBroadcastEvent(JSContext* cx, uint argc, jsval* argv);
void FireItemActionEvent(DWORD GID, char* Code, BYTE Mode, bool Global);
bool FireGamePacketEvent(BYTE* pPacket, DWORD dwSize);
bool FireGamePacketSentEvent(BYTE* pPacket, DWORD dwSize);
bool FireRealmPacketEvent(BYTE* pPacket, DWORD dwSize);

void ReleaseGameLock(void);
void TakeGameLock(void);

struct ChatEventHelper {
  const char *name, *nick;
  const wchar_t* msg;
};

struct CopyDataHelper {
  DWORD mode;
  wchar_t* msg;
};

struct ItemEventHelper {
  DWORD id;
  char* code;
  WORD mode;
  bool global;
};

struct KeyEventHelper {
  BOOL up;
  WPARAM key;
};

struct GameActionEventHelper {
  BYTE mode;
  DWORD param1, param2;
  char* name1;
  wchar_t* name2;
};

struct SingleArgHelper {
  DWORD arg1;
};

struct DoubleArgHelper {
  DWORD arg1, arg2;
};

struct DoubleArgEvent : Event {
  DWORD arg1, arg2;
};

struct TripleArgHelper {
  DWORD arg1, arg2, arg3;
};

struct TripleArgEvent : Event {
  DWORD arg1, arg2, arg3;
};

struct QuadArgHelper {
  DWORD arg1, arg2, arg3, arg4;
};

struct QuadArgEvent : Event {
  DWORD arg1, arg2, arg3, arg4;
};

struct BCastEventHelper {
  JSContext* cx;
  jsval* argv;
  uint argc;
};

struct PacketEventHelper {
  const char* name;
  BYTE* pPacket;
  DWORD dwSize;
};
