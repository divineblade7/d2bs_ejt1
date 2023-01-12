#pragma once

#include "d2bs/script/AutoRoot.h"
#include "d2bs/script/Script.h"
#include "d2bs/script/js32.h"

#include <list>
#include <map>
#include <mutex>
#include <string>

typedef std::map<std::wstring, Script*> ScriptMap;

enum EngineState { Starting, Running, Paused, Stopping, Stopped };

class ScriptEngine {
 public:
  // should this use friendship? ~ ejt
  friend class Script;

  ScriptEngine() noexcept = default;
  ~ScriptEngine() noexcept = default;

  ScriptEngine(const ScriptEngine&) = delete;
  ScriptEngine& operator=(const ScriptEngine&) = delete;

  static ScriptEngine* __instance() {
    static ScriptEngine instance;
    return &instance;
  }

  bool init();
  void shutdown();
  void StopAll(bool forceStop = false);

  // TODO: Figure out what this function actually does and if we need it. ~ ejt
  void FlushCache(void);

  Script* CompileFile(const wchar_t* file, ScriptType type, uint32_t argc = 0, JSAutoStructuredCloneBuffer** argv = NULL,
                      bool recompile = false);
  void RunCommand(const wchar_t* command);
  void DisposeScript(Script* script);

  std::unique_lock<std::mutex> lock_script_list(const char* loc);

  // TODO: Add constraint to Fn. Signature should be fn(Script*, Args...)
  template <typename Fn, typename... Args>
  bool for_each(Fn fn, Args&&... args) {
    auto lock = lock_script_list("for_each");

    bool block = false;
    for (const auto& [_, script] : scripts_) {
      block |= fn(script, std::forward<Args>(args)...);
    }
    return block;
  }

  unsigned int GetCount(bool active = true, bool unexecuted = false);

  int AddDelayedEvent(std::shared_ptr<TimeoutEvent> evt, int freq);
  void RemoveDelayedEvent(int key);

  EngineState state() {
    return state_;
  }

  ScriptMap& scripts() {
    return scripts_;
  }

 private:
  EngineState state_ = Stopped;

  ScriptMap scripts_{};
  std::mutex script_list_mutex_{};

  // To be removed
  // implement timer into TimeoutEvent that can be used to delay processing of event
  std::list<std::shared_ptr<TimeoutEvent>> DelayedExecList_;
  int delayedExecKey_;
};

#define sScriptEngine ScriptEngine::__instance()

// gcCallback(JSContext* cx, JSGCStatus status);
void CALLBACK EventTimerProc(LPVOID lpArg, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
