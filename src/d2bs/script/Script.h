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

  // private these
  DWORD threadId;
  FunctionMap functions;

  void Run(void);
  void Join(void);
  void Pause(void);
  void Resume(void);
  bool IsPaused(void);
  bool BeginThread(LPTHREAD_START_ROUTINE ThreadFunc);
  void RunCommand(const wchar_t* command);
  inline void SetPauseState(bool reallyPaused) {
    isReallyPaused = reallyPaused;
  }
  inline bool IsReallyPaused(void) {
    return isReallyPaused;
  }
  void Stop(bool force = false, bool reallyForce = false);

  inline const wchar_t* GetFilename(void) {
    return fileName.c_str();
  }
  const wchar_t* GetShortFilename(void);
  inline JSContext* GetContext(void) {
    return context;
  }
  inline JSRuntime* GetRuntime(void) {
    return runtime;
  }
  inline JSObject* GetGlobalObject(void) {
    return globalObject;
  }
  inline JSObject* GetScriptObject(void) {
    return scriptObject;
  }
  inline ScriptState GetState(void) {
    return scriptState;
  }
  inline void TriggerOperationCallback(void) {
    if (hasActiveCX) JS_TriggerOperationCallback(runtime);
  }
  int GetExecutionCount(void);
  DWORD GetThreadId(void);
  DWORD LastGC;
  bool hasActiveCX;  // hack to get away from JS_IsRunning
  HANDLE eventSignal;

  // UGLY HACK to fix up the player gid on game join for cached scripts/oog scripts
  void UpdatePlayerGid(void);
  // Hack. Include from console needs to run on the RunCommandThread / cx.
  //		 a better solution may be to keep a list of threadId / cx and have a GetCurrentThreadCX()
  inline void SetContext(JSContext* cx) {
    context = cx;
  }
  bool IsRunning(void);
  bool IsAborted(void);
  void Lock() {
    EnterCriticalSection(&lock);
  }  // needed for events walking function list
  void Unlock() {
    LeaveCriticalSection(&lock);
  }
  bool IsIncluded(const wchar_t* file);
  bool Include(const wchar_t* file);

  bool IsListenerRegistered(const char* evtName);
  void RegisterEvent(const char* evtName, jsval evtFunc);
  bool IsRegisteredEvent(const char* evtName, jsval evtFunc);
  void UnregisterEvent(const char* evtName, jsval evtFunc);
  void ClearEvent(const char* evtName);
  void ClearAllEvents(void);
  void FireEvent(Event*);
  std::list<Event*> EventList;

 private:
  std::wstring fileName;
  int execCount;
  ScriptState scriptState;
  JSContext* context;
  JSScript* script;
  JSRuntime* runtime;
  myUnit* me;
  uint argc;
  JSAutoStructuredCloneBuffer** argv;

  JSObject *globalObject, *scriptObject;
  bool isLocked, isPaused, isReallyPaused, isAborted;

  IncludeList includes, inProgress;

  HANDLE threadHandle;

  CRITICAL_SECTION lock;
};

struct RUNCOMMANDSTRUCT {
  Script* script;
  const wchar_t* command;
};

DWORD WINAPI RunCommandThread(void* data);
DWORD WINAPI ScriptThread(void* data);
