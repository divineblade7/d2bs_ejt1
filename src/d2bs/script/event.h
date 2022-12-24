#pragma once

#include "d2bs/script/AutoRoot.h"

#include <Windows.h>
#include <list>
#include <map>

class Script;

typedef std::list<AutoRoot*> FunctionList;
typedef std::map<std::string, FunctionList> FunctionMap;

struct Event {
  Event() : count(0){};
  Script* owner;
  JSObject* object;
  FunctionList functions;
  JSAutoStructuredCloneBuffer** argv;
  uint argc;
  char* name;
  void* arg1;
  void* arg2;
  void* arg3;
  void* arg4;
  void* arg5;
  volatile long count;
  inline void threadFinished() {
    // clean up after both threads are done with the event
    char* evtName = (char*)name;
    InterlockedIncrement(&count);
    if (count > 1) {
      Event* evt = this;

      if (strcmp(evtName, "itemaction") == 0) {
        delete arg1;
        free(arg2);
        delete arg3;
        delete arg4;
      }
      if (strcmp(evtName, "gameevent") == 0) {
        delete evt->arg1;
        delete evt->arg2;
        delete evt->arg3;
        free(evt->arg4);
        free(evt->arg5);
      }
      if (strcmp(evtName, "copydata") == 0) {
        delete evt->arg1;
        free(evt->arg2);
      }
      if (strcmp(evtName, "chatmsg") == 0 || strcmp(evtName, "chatinput") == 0 || strcmp(evtName, "whispermsg") == 0 ||
          strcmp(evtName, "chatmsgblocker") == 0 || strcmp(evtName, "chatinputblocker") == 0 ||
          strcmp(evtName, "whispermsgblocker") == 0) {
        free(evt->arg1);
        free(evt->arg2);
        delete evt->arg4;
      }
      if (strcmp(evtName, "mousemove") == 0 || strcmp(evtName, "ScreenHookHover") == 0) {
        delete evt->arg1;
        delete evt->arg2;
      }
      if (strcmp(evtName, "mouseclick") == 0) {
        delete evt->arg1;
        delete evt->arg2;
        delete evt->arg3;
        delete evt->arg4;
      }
      if (strcmp(evtName, "keyup") == 0 || strcmp(evtName, "keydownblocker") == 0 || strcmp(evtName, "keydown") == 0 ||
          strcmp(evtName, "memana") == 0 || strcmp(evtName, "melife") == 0 || strcmp(evtName, "playerassign") == 0) {
        delete evt->arg1;
        delete evt->arg4;
      }
      if (strcmp(evtName, "ScreenHookClick") == 0) {
        delete evt->arg1;
        delete evt->arg2;
        delete evt->arg3;
        delete evt->arg4;
      }
      if (strcmp(evtName, "Command") == 0) {
        // cleaned up in ExecScriptEvent
      }
      if (strcmp(evtName, "scriptmsg") == 0) {
        delete evt->arg1;
      }
      if (strcmp(evtName, "gamepacket") == 0 || strcmp(evtName, "gamepacketsent") == 0 ||
          strcmp(evtName, "realmpacket") == 0) {
        delete[] evt->arg1;
        delete evt->arg2;
        delete evt->arg4;
      }

      free(evt->name);
      delete evt;
      Event::~Event();
    }
  };
};

bool ChatEvent(const char* lpszNick, const wchar_t* lpszMsg);
bool ChatInputEvent(wchar_t* lpszMsg);
void LifeEvent(DWORD dwLife);
void ManaEvent(DWORD dwMana);
void CopyDataEvent(DWORD dwMode, wchar_t* lpszMsg);
void GameActionEvent(BYTE mode, DWORD param1, DWORD param2, char* name1, wchar_t* name2);
bool WhisperEvent(const char* lpszNick, const wchar_t* lpszMsg);
bool KeyDownUpEvent(WPARAM bByte, BYTE bUp);
void PlayerAssignEvent(DWORD dwUnitId);
void MouseClickEvent(int button, POINT pt, bool bUp);
void MouseMoveEvent(POINT pt);
void ScriptBroadcastEvent(JSContext* cx, uint argc, jsval* argv);
void ItemActionEvent(DWORD GID, char* Code, BYTE Mode, bool Global);
bool GamePacketEvent(BYTE* pPacket, DWORD dwSize);
bool GamePacketSentEvent(BYTE* pPacket, DWORD dwSize);
bool RealmPacketEvent(BYTE* pPacket, DWORD dwSize);

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

struct TripleArgHelper {
  DWORD arg1, arg2, arg3;
};
struct QuadArgHelper {
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
