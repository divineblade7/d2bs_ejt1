#pragma once

#include "d2bs/script/AutoRoot.h"

#include <Windows.h>
#include <list>
#include <map>

class Script;

typedef std::list<AutoRoot*> FunctionList;
typedef std::map<std::string, FunctionList> FunctionMap;

class Event {
 public:
  // We don't process enough events for the overhead of polymorphic class to be a bottleneck
  virtual void process() = 0;

  Script* owner = nullptr;
  FunctionList functions;
  std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> args;
  std::string name;
  bool block = false;
};

class CopyDataEvent : public Event {
 public:
  void process();

  DWORD mode;
  std::wstring msg;
};

struct CommandEvent : public Event {
 public:
  void process();

  std::wstring command;
};

struct ChatEvent : public Event {
 public:
  void process();

  std::string name1, nick;
  std::wstring msg;
};

struct PacketEvent : public Event {
 public:
  void process();

  std::string name1;
  std::vector<uint8_t> bytes;
};

struct BroadcastEvent : public Event {
 public:
  void process();
};

struct GameActionEvent : public Event {
 public:
  void process();

  BYTE mode;
  DWORD param1, param2;
  std::string name1;
  std::wstring name2;
};

struct KeyEvent : public Event {
 public:
  void process();

  BOOL up;
  WPARAM key;
};

struct ItemEvent : public Event {
 public:
  void process();

  DWORD id;
  std::string code;
  WORD mode;
  bool global;
};

struct TimeoutEvent : public Event {
 public:
  void process();

  int key;
  HANDLE handle;
  jsval* val;
};

struct LifeEvent : public Event {
 public:
  void process();

  DWORD life;
};

struct ManaEvent : public Event {
 public:
  void process();

  DWORD mana;
};

struct PlayerAssignEvent : public Event {
 public:
  void process();

  DWORD unit_id;
};

struct MouseClickEvent : public Event {
 public:
  void process();

  DWORD button;
  DWORD x;
  DWORD y;
  DWORD up;
};

struct ScreenHookClickEvent : public Event {
 public:
  void process();

  int button;
  LONG x;
  LONG y;
};

struct MouseMoveEvent : public Event {
 public:
  void process();

  DWORD x;
  DWORD y;
};

struct DisposeEvent : public Event {
 public:
  void process();
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
