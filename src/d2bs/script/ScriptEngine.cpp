#include "d2bs/script/ScriptEngine.h"

#include "d2bs/core/Core.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/engine.h"
#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/api/JSGlobalFuncs.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/script/event.h"
#include "d2bs/utils/Console.h"
#include "d2bs/utils/Helpers.h"

#include <algorithm>
#include <assert.h>
#include <vector>

// internal ForEachScript helper functions
bool __fastcall DisposeScript(Script* script, ScriptEngine* engine) {
  engine->DisposeScript(script);
  return true;
}

bool __fastcall StopScript(Script* script, ScriptEngine* engine, bool force) {
  script->request_interrupt();
  if (script->type() != ScriptType::Command) {
    script->stop(force, engine->state() == Stopping);
  }
  return true;
}

bool ScriptEngine::init() {
  if (state_ == Stopped) {
    state_ = Starting;
    auto lock = lock_script_list("startup - enter");

    Script* console = nullptr;
    if (wcslen(Vars.settings.szConsole) > 0) {
      auto path = (Vars.settings.script_dir / Vars.settings.szConsole).make_preferred().wstring();
      console = new Script(this, path.c_str(), ScriptType::Command);
    } else {
      console = new Script(this, L"", ScriptType::Command);
    }
    console->BeginThread(ScriptThread);
    scripts_[L"console"] = console;
    state_ = Running;
  }
  return TRUE;
}

void ScriptEngine::shutdown() {
  if (state_ == Running) {
    // bring the engine down properly
    auto lock = lock_script_list("Shutdown");

    state_ = Stopping;
    StopAll(true);

    // clear all scripts now that they're stopped
    for_each(::DisposeScript, this);

    if (!scripts_.empty()) {
      scripts_.clear();
    }

    JS_ShutDown();
    state_ = Stopped;
  }
}

void ScriptEngine::StopAll(bool forceStop) {
  if (state_ == Running) {
    for_each(::StopScript, this, forceStop);
  }
}

void ScriptEngine::FlushCache(void) {
  if (state_ != Running) {
    return;
  }

  static bool isFlushing = false;

  if (isFlushing || Vars.settings.bDisableCache) {
    return;
  }

  // EnterCriticalSection(&lock);
  // TODO: examine if this lock is necessary any more
  EnterCriticalSection(&Vars.cFlushCacheSection);

  isFlushing = true;

  for_each(::DisposeScript, this);

  isFlushing = false;

  LeaveCriticalSection(&Vars.cFlushCacheSection);
  // LeaveCriticalSection(&lock);
}

Script* ScriptEngine::CompileFile(const wchar_t* file, ScriptType type, uint argc, JSAutoStructuredCloneBuffer** argv,
                                  bool) {
  if (state_ != Running) {
    return NULL;
  }

  wchar_t* fileName = _wcsdup(file);
  _wcslwr_s(fileName, wcslen(file) + 1);

  try {
    if (scripts_.count(fileName)) {
      scripts_[fileName]->stop();
    }

    Script* script = new Script(this, fileName, type, argc, argv);
    scripts_[fileName] = script;
    free(fileName);
    return script;
  } catch (std::exception e) {
    wchar_t* what = AnsiToUnicode(e.what());
    Print(what);
    delete[] what;
    free(fileName);
    return NULL;
  }
}

void ScriptEngine::RunCommand(const wchar_t* command) {
  if (state_ != Running) {
    return;
  }

  try {
    auto lock = lock_script_list("RunCommand");
    if (scripts_.contains(L"console")) {
      scripts_[L"console"]->RunCommand(command);
    }
  } catch (std::exception e) {
    wchar_t* what = AnsiToUnicode(e.what());
    Print(what);
    delete[] what;
  }
}

void ScriptEngine::DisposeScript(Script* script) {
  auto lock = lock_script_list("DisposeScript");

  const wchar_t* nFilename = script->filename();

  if (scripts_.count(nFilename)) {
    scripts_.erase(nFilename);
  }

  if (GetCurrentThreadId() == script->thread_id_) {
    delete script;
  } else {
    // bad things happen if we delete from another thread
    auto evt = std::make_shared<DisposeEvent>(script);
    script->FireEvent(evt);
  }
}

std::unique_lock<std::mutex> ScriptEngine::lock_script_list(const char*) {
  std::unique_lock<std::mutex> lock(script_list_mutex_);
  return std::move(lock);
  // Log(loc);
}

unsigned int ScriptEngine::GetCount(bool active, bool unexecuted) {
  if (state_ != Running) {
    return 0;
  }

  auto lock = lock_script_list("getCount");

  int count = scripts_.size();
  for (const auto& [_, script] : scripts_) {
    if ((!active && script->is_running() && !script->is_stopped()) ||
        !unexecuted && script->GetExecutionCount() == 0 && !script->is_running()) {
      --count;
    }
  }

  // assert(count >= 0);
  return count;
}

int ScriptEngine::AddDelayedEvent(std::shared_ptr<TimeoutEvent> evt, int freq) {
  delayedExecKey_++;
  evt->key = delayedExecKey_;
  evt->handle = CreateWaitableTimer(NULL, true, NULL);

  __int64 start;
  start = freq * -10000;
  LARGE_INTEGER lStart;
  // Copy the relative time into a LARGE_INTEGER.
  lStart.LowPart = (DWORD)(start & 0xFFFFFFFF);
  lStart.HighPart = (LONG)(start >> 32);
  freq = (evt->name() == "setInterval") ? freq : 0;
  EnterCriticalSection(&Vars.cEventSection);
  DelayedExecList_.push_back(evt);
  Log(L"&evt could crash");
  Print(L"&evt could crash");
  SetWaitableTimer((HANDLE*)evt->handle, &lStart, freq, &EventTimerProc, &evt, false);
  LeaveCriticalSection(&Vars.cEventSection);

  return delayedExecKey_;
}

void ScriptEngine::RemoveDelayedEvent(int key) {
  std::list<std::shared_ptr<TimeoutEvent>>::iterator it;
  it = DelayedExecList_.begin();
  while (it != DelayedExecList_.end()) {
    if ((*it)->key == key) {
      CancelWaitableTimer((*it)->handle);
      CloseHandle((*it)->handle);
      std::shared_ptr<TimeoutEvent> evt = *it;
      evt->owner()->UnregisterEvent(evt->name().c_str(), *evt->val);
      delete evt->val;
      it = DelayedExecList_.erase(it);
    } else {
      it++;
    }
  }
  LeaveCriticalSection(&Vars.cEventSection);
}

void CALLBACK EventTimerProc(LPVOID lpArg, DWORD, DWORD) {
  std::shared_ptr<Event>* evt = (std::shared_ptr<Event>*)lpArg;
  (*evt)->owner()->FireEvent(*evt);
}
