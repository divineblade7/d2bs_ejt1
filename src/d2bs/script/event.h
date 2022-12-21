#pragma once

#include "d2bs/script/AutoRoot.h"

#include <map>
#include <list>

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
