#pragma once

#include "d2bs/script/AutoRoot.h"

#include <Windows.h>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

class Script;

typedef std::list<AutoRoot*> FunctionList;
typedef std::map<std::string, FunctionList> FunctionMap;

class Event {
 public:
  Event(Script* owner, std::string name);

  // We don't process enough events for the overhead of polymorphic class to be a bottleneck
  virtual void process() = 0;

  /**
   * @brief Blocking function that waits for the event to be processed.
   * @todo: Add a way to wait for X milliseconds.
   */
  void wait();

  Script* owner() {
    return owner_;
  }

  const std::string& name() {
    return name_;
  }

  bool block() {
    return block_;
  }

 protected:
  /**
   * @brief Notifies all threads waiting for this event that we are done processing.
   */
  void notify_all();

 protected:
  Script* owner_ = nullptr;
  std::string name_;
  bool block_ = false;
  std::atomic_bool is_processed_ = false;
};

class CopyDataEvent : public Event {
 public:
  CopyDataEvent(Script* owner, DWORD mode_, std::wstring msg_);

  void process();

  DWORD mode;
  std::wstring msg;
};

struct CommandEvent : public Event {
 public:
  CommandEvent(Script* owner, std::wstring command_);

  void process();

  std::wstring command;
};

struct ChatEvent : public Event {
 public:
  ChatEvent(Script* owner, const char* name, std::string name1_, std::string nick_, std::wstring msg_);

  void process();

  std::string name1, nick;
  std::wstring msg;
};

struct PacketEvent : public Event {
 public:
  PacketEvent(Script* owner, std::string name1_, std::vector<uint8_t> bytes_);

  void process();

  std::string name1;
  std::vector<uint8_t> bytes;
};

struct BroadcastEvent : public Event {
 public:
  BroadcastEvent(Script* owner, std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> args_);

  void process();

  std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> args;
};

struct GameActionEvent : public Event {
 public:
  GameActionEvent(Script* owner, BYTE mode_, DWORD param1_, DWORD param2_, std::string name1_, std::wstring name2_);

  void process();

  BYTE mode;
  DWORD param1, param2;
  std::string name1;
  std::wstring name2;
};

struct KeyEvent : public Event {
 public:
  KeyEvent(Script* owner, WPARAM key_, BYTE bUp);

  void process();

  BOOL up;
  WPARAM key;
};

struct KeyBlockEvent : public Event {
 public:
  KeyBlockEvent(Script* owner, WPARAM key_, BYTE bUp);

  void process();

  BOOL up;
  WPARAM key;
};

struct ItemEvent : public Event {
 public:
  ItemEvent(Script* owner, DWORD id_, std::string code_, WORD mode_, bool global_);

  void process();

  DWORD id;
  std::string code;
  WORD mode;
  bool global;
};

struct TimeoutEvent : public Event {
 public:
  TimeoutEvent(Script* owner, const char* name, jsval* val_);

  void process();

  int key;
  HANDLE handle;
  jsval* val;
};

struct LifeEvent : public Event {
 public:
  LifeEvent(Script* owner, DWORD life_);

  void process();

  DWORD life;
};

struct ManaEvent : public Event {
 public:
  ManaEvent(Script* owner, DWORD mana_);

  void process();

  DWORD mana;
};

struct PlayerAssignEvent : public Event {
 public:
  PlayerAssignEvent(Script* owner, DWORD unitid);

  void process();

  DWORD unit_id;
};

struct MouseClickEvent : public Event {
 public:
  MouseClickEvent(Script* owner, DWORD button_, DWORD x_, DWORD y_, DWORD up_);

  void process();

  DWORD button;
  DWORD x;
  DWORD y;
  DWORD up;
};

struct ScreenHookClickEvent : public Event {
 public:
  ScreenHookClickEvent(Script* owner, FunctionList funcs, int button_, LONG x_, LONG y_);

  void process();

  int button;
  LONG x;
  LONG y;
  FunctionList functions;
};

struct ScreenHookHoverEvent : public Event {
 public:
  ScreenHookHoverEvent(Script* owner, FunctionList funcs, LONG x_, LONG y_);

  void process();

  LONG x;
  LONG y;
  FunctionList functions;
};

struct MouseMoveEvent : public Event {
 public:
  MouseMoveEvent(Script* owner, DWORD x_, DWORD y_);

  void process();

  DWORD x;
  DWORD y;
};

struct DisposeEvent : public Event {
 public:
  DisposeEvent(Script* owner);

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
