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

enum class ScriptType { InGame, OutOfGame, Command };
enum class ScriptState { Stopped, Running, Paused };

// This should be typdef'd inside Script but is used by JSSandbox
typedef std::map<std::wstring, bool> IncludeList;

class Script {
  Script(const wchar_t* file, ScriptType type, uint argc = 0, JSAutoStructuredCloneBuffer** argv = NULL);
  ~Script();

  Script(const Script&) = delete;
  Script& operator=(const Script&) = delete;

 public:
  // not sure if we want to keep using friend class ~ ejt
  friend class ScriptEngine;

  void Run();
  void Join();
  void Pause();
  void Resume();
  bool is_running();
  bool is_paused();
  bool is_stopped();

  bool BeginThread(LPTHREAD_START_ROUTINE ThreadFunc);
  void RunCommand(const wchar_t* command);
  void Stop(bool force = false, bool reallyForce = false);

  int GetExecutionCount();

  // UGLY HACK to fix up the player gid on game join for cached scripts/oog scripts
  void UpdatePlayerGid();

  bool IsIncluded(const wchar_t* file);
  bool Include(const wchar_t* file);

  bool IsListenerRegistered(const char* evtName);
  void RegisterEvent(const char* evtName, jsval evtFunc);
  bool IsRegisteredEvent(const char* evtName, jsval evtFunc);
  void UnregisterEvent(const char* evtName, jsval evtFunc);
  void ClearEvent(const char* evtName);
  void ClearAllEvents();
  void FireEvent(Event*);

  inline const wchar_t* filename() {
    return filename_.c_str();
  }

  const wchar_t* filename_short();

  inline JSContext* context() {
    return context_;
  }

  inline JSRuntime* runtime() {
    return runtime_;
  }

  inline ScriptType type() {
    return type_;
  }

  inline void TriggerOperationCallback() {
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
  JSRuntime* runtime_ = nullptr;
  JSContext* context_ = nullptr;
  JSScript* script_ = nullptr;
  JSObject* globals_ = nullptr;

  JSAutoStructuredCloneBuffer** argv_;
  uint argc_;

  ScriptType type_;
  ScriptState state_ = ScriptState::Stopped;

  std::wstring filename_;
  int execCount_ = 0;
  myUnit* me_ = nullptr;
  std::list<Event*> EventList_;

  IncludeList includes_;
  IncludeList inProgress_;

  HANDLE thread_handle_ = INVALID_HANDLE_VALUE;
  DWORD thread_id_ = 0;

  CRITICAL_SECTION lock_;

  FunctionMap functions_;

  DWORD LastGC_ = 0;
  bool hasActiveCX_ = false;  // hack to get away from JS_IsRunning
  HANDLE eventSignal_;
};

struct RUNCOMMANDSTRUCT {
  Script* script;
  const wchar_t* command;
};

DWORD WINAPI RunCommandThread(void* data);
DWORD WINAPI ScriptThread(void* data);
