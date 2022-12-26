#pragma once

#include "d2bs/script/AutoRoot.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/script/event.h"
#include "d2bs/script/js32.h"
#include "d2bs/utils/mpmc_queue.h"

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <windows.h>

DWORD WINAPI ScriptThread(void* data);
void reportError(JSContext* cx, const char* message, JSErrorReport* report);
JSBool operationCallback(JSContext* cx);
JSBool contextCallback(JSContext* cx, uint contextOp);

enum class ScriptType { InGame, OutOfGame, Command };
enum class ScriptState { Stopped, Running, Paused };

// This should be typdef'd inside Script but is used by JSSandbox
typedef std::map<std::wstring, bool> IncludeList;
class ScriptEngine;

class Script {
  // Script is aware of the ScriptEngine which owns the Script. This behavior removes the need for a global ScriptEngine
  // variable, however it also means that Script depends on ScriptEngine which may not be the behavior we want here.
  // The ideal condition is that Script is completely removed from ScriptEngine and operates independently from it.
  Script(ScriptEngine* engine, const wchar_t* file, ScriptType type, uint argc = 0,
         JSAutoStructuredCloneBuffer** argv = NULL);
  ~Script();

  Script(const Script&) = delete;
  Script& operator=(const Script&) = delete;

 public:
  // not sure if we want to keep using friend class ~ ejt
  friend class ScriptEngine;

  void run();
  void stop(bool force = false, bool reallyForce = false);

  void join();
  void pause();
  void resume();
  bool is_running();
  bool is_paused();
  bool is_stopped();

  bool BeginThread(LPTHREAD_START_ROUTINE ThreadFunc);
  void RunCommand(const wchar_t* command);

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

  // possibly a way to instantly process the event
  void FireEvent(std::shared_ptr<Event>);

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

  inline void request_interrupt() {
    if (hasActiveCX_) {
      if (GetCurrentThreadId() == thread_id_) {
        operationCallback(context_);
      } else {
        JS_TriggerOperationCallback(runtime_);
      }
    }
  }

  DWORD thread_id() {
    return (thread_handle_ == INVALID_HANDLE_VALUE ? -1 : thread_id_);
  }

  FunctionMap& functions() {
    return functions_;
  }

  DWORD last_gc() {
    return last_gc_;
  }

  void set_last_gc(DWORD val) {
    last_gc_ = val;
  }

  void set_has_active_cx(bool val) {
    hasActiveCX_ = val;
  }

  friend JSBool operationCallback(JSContext* cx);
  friend JSBool contextCallback(JSContext* cx, uint contextOp);

 private:
  /**
   * @brief This must be called from the same thread that created JS runtime and context!
   */
  void process_events();

 private:
  ScriptEngine* engine_ = nullptr;
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

  IncludeList includes_;
  IncludeList inProgress_;

  HANDLE thread_handle_ = INVALID_HANDLE_VALUE;
  DWORD thread_id_ = 0;

  std::mutex mutex_;

  FunctionMap functions_;

  DWORD last_gc_ = 0;
  bool hasActiveCX_ = false;  // hack to get away from JS_IsRunning

  d2bs::mpmc_queue<std::shared_ptr<Event>> event_queue_;  // new event system ~ ejt
};
