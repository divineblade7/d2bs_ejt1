#pragma once

#include "d2bs/script/AutoRoot.h"
#include "d2bs/script/Script.h"
#include "d2bs/script/js32.h"

#include <list>
#include <map>
#include <string>

typedef std::map<std::wstring, Script*> ScriptMap;

typedef bool(__fastcall* ScriptCallback)(Script*, void*, uint);

enum EngineState { Starting, Running, Paused, Stopping, Stopped };

class ScriptEngine {
  ScriptEngine() noexcept = default;
  ~ScriptEngine() noexcept = default;

 public:
  // should this use friendship? ~ ejt
  friend class Script;

  // private these
  ScriptMap scripts{};
  CRITICAL_SECTION lock{};

  ScriptEngine(const ScriptEngine&) = delete;
  ScriptEngine& operator=(const ScriptEngine&) = delete;

  static ScriptEngine* instance() {
    static ScriptEngine _instance;
    return &_instance;
  }

  BOOL Startup(void);
  void Shutdown(void);
  EngineState GetState(void) {
    return state;
  }

  void FlushCache(void);

  Script* CompileFile(const wchar_t* file, ScriptState state, uint argc = 0, JSAutoStructuredCloneBuffer** argv = NULL,
                      bool recompile = false);
  void RunCommand(const wchar_t* command);
  void DisposeScript(Script* script);

  void LockScriptList(const char* loc);
  void UnLockScriptList(const char* loc);

  bool ForEachScript(ScriptCallback callback, void* argv, uint argc);
  unsigned int GetCount(bool active = true, bool unexecuted = false);

  JSRuntime* GetRuntime(void) {
    return runtime;
  }
  JSContext* GetGlobalContext(void) {
    return context;
  }

  void InitClass(JSContext* context, JSObject* globalObject, JSClass* classp, JSFunctionSpec* methods,
                 JSPropertySpec* props, JSFunctionSpec* s_methods, JSPropertySpec* s_props);
  void DefineConstant(JSContext* context, JSObject* globalObject, const char* name, int value);

  void StopAll(bool forceStop = false);
  void UpdateConsole();

  int AddDelayedEvent(Event* evt, int freq);
  void RemoveDelayedEvent(int key);

 private:
  JSRuntime* runtime = nullptr;
  JSContext* context = nullptr;
  Script* console = nullptr;
  EngineState state = Stopped;
  std::list<Event*> DelayedExecList;
  int delayedExecKey;
  CRITICAL_SECTION scriptListLock{};
};

#define sScriptEngine ScriptEngine::instance()

// these ForEachScript helpers are exposed in case they can be of use somewhere
bool __fastcall StopIngameScript(Script* script, void*, uint);

struct EventHelper {
  char* evtName;
  AutoRoot** argv;
  uint argc;
  bool executed;
};

JSBool operationCallback(JSContext* cx);
JSBool contextCallback(JSContext* cx, uint contextOp);
// gcCallback(JSContext* cx, JSGCStatus status);
void reportError(JSContext* cx, const char* message, JSErrorReport* report);
bool ExecScriptEvent(Event* evt, bool clearList);
void CALLBACK EventTimerProc(LPVOID lpArg, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
