#pragma once

#include "d2bs/script/AutoRoot.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/script/event.h"
#include "d2bs/script/js32.h"
#include "d2bs/utils/Events.h"

#include <list>
#include <map>
#include <string>
#include <windows.h>

enum ScriptState { InGame, OutOfGame, Command };

typedef std::map<std::wstring, bool> IncludeList;
typedef std::list<Script*> ScriptList;

class Script {
  Script(const wchar_t* file, ScriptState state, uint argc = 0, JSAutoStructuredCloneBuffer** argv = NULL);
  ~Script(void);

  Script(const Script&) = delete;
  Script& operator=(const Script&) = delete;

 public:
  // not sure if we want to keep using friend class ~ ejt
  friend class ScriptEngine;

  void Run(void);
  void Join(void);
  void Pause(void);
  void Resume(void);
  bool IsPaused(void);
  bool BeginThread(LPTHREAD_START_ROUTINE ThreadFunc);
  void RunCommand(const wchar_t* command);
  void Stop(bool force = false, bool reallyForce = false);

  int GetExecutionCount(void);

  // UGLY HACK to fix up the player gid on game join for cached scripts/oog scripts
  void UpdatePlayerGid(void);

  bool IsRunning(void);
  bool IsAborted(void);

  bool IsIncluded(const wchar_t* file);
  bool Include(const wchar_t* file);

  bool IsListenerRegistered(const char* evtName);
  void RegisterEvent(const char* evtName, jsval evtFunc);
  bool IsRegisteredEvent(const char* evtName, jsval evtFunc);
  void UnregisterEvent(const char* evtName, jsval evtFunc);
  void ClearEvent(const char* evtName);
  void ClearAllEvents(void);
  void FireEvent(Event*);

  // Hack. Include from console needs to run on the RunCommandThread / cx.
  //		 a better solution may be to keep a list of threadId / cx and have a GetCurrentThreadCX()
  inline void SetContext(JSContext* cx) {
    context_ = cx;
  }

  inline void SetPauseState(bool reallyPaused) {
    isReallyPaused_ = reallyPaused;
  }

  inline bool IsReallyPaused(void) {
    return isReallyPaused_;
  }

  inline const wchar_t* GetFilename(void) {
    return fileName_.c_str();
  }

  const wchar_t* GetShortFilename(void);
  inline JSContext* GetContext(void) {
    return context_;
  }

  inline JSRuntime* GetRuntime(void) {
    return runtime_;
  }

  inline JSObject* GetGlobalObject(void) {
    return globalObject_;
  }

  inline JSObject* GetScriptObject(void) {
    return scriptObject_;
  }

  inline ScriptState GetState(void) {
    return scriptState_;
  }

  inline void TriggerOperationCallback(void) {
    if (hasActiveCX_) {
      JS_TriggerOperationCallback(runtime_);
    }
  }

  std::list<Event*>& events() {
    return EventList_;
  }

  DWORD thread_id() {
    return (thread_handle_ == INVALID_HANDLE_VALUE ? -1 : thread_id_);
  }

  FunctionMap& functions() {
    return functions_;
  }

  DWORD last_gc() {
    return LastGC_;
  }

  void set_last_gc(DWORD val) {
    LastGC_ = val;
  }

  bool get_has_active_cx() {
    return hasActiveCX_;
  }

  void set_has_active_cx(bool val) {
    hasActiveCX_ = val;
  }

  HANDLE event_signal() {
    return eventSignal_;
  }

 private:
  std::wstring fileName_;
  int execCount_;
  ScriptState scriptState_;
  JSContext* context_;
  JSScript* script_;
  JSRuntime* runtime_;
  myUnit* me_;
  uint argc_;
  JSAutoStructuredCloneBuffer** argv_;
  std::list<Event*> EventList_;

  JSObject *globalObject_, *scriptObject_;
  bool isLocked_, isPaused_, isReallyPaused_, isAborted_;

  IncludeList includes_, inProgress_;

  HANDLE thread_handle_ = INVALID_HANDLE_VALUE;
  DWORD thread_id_ = 0;

  CRITICAL_SECTION lock_;

  FunctionMap functions_;

  DWORD LastGC_;
  bool hasActiveCX_;  // hack to get away from JS_IsRunning
  HANDLE eventSignal_;
};

struct RUNCOMMANDSTRUCT {
  Script* script;
  const wchar_t* command;
};

DWORD WINAPI RunCommandThread(void* data);
DWORD WINAPI ScriptThread(void* data);
