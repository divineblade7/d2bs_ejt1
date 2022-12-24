#include "d2bs/script/Script.h"

#include "d2bs/core/Core.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/engine.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/script/api/JSGlobalFuncs.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/utils/Helpers.h"

#include <algorithm>
#include <io.h>

Script::Script(ScriptEngine* engine, const wchar_t* file, ScriptType type, uint argc,
               JSAutoStructuredCloneBuffer** argv)
    : engine_(engine), type_(type), argv_(argv), argc_(argc) {
  event_signal_ = CreateEvent(nullptr, true, false, nullptr);

  if (type_ == ScriptType::Command && wcslen(file) < 1) {
    filename_ = std::wstring(L"Command Line");
  } else {
    if (_waccess(file, 0) != 0) {
      DEBUG_LOG(file);

      throw std::exception("File not found");
    }

    wchar_t* tmpName = _wcsdup(file);
    if (!tmpName) {
      throw std::exception("Could not dup filename");
    }

    _wcslwr_s(tmpName, wcslen(file) + 1);
    filename_ = std::wstring(tmpName);
    replace(filename_.begin(), filename_.end(), L'/', L'\\');
    free(tmpName);
  }
}

Script::~Script() {
  state_ = ScriptState::Stopped;
  hasActiveCX_ = false;

  if (JS_IsInRequest(runtime_)) {
    JS_EndRequest(context_);
  }

  std::lock_guard<std::mutex> lock(mutex_);

  JS_DestroyContext(context_);
  JS_DestroyRuntime(runtime_);

  context_ = nullptr;
  globals_ = nullptr;
  script_ = nullptr;
  CloseHandle(event_signal_);
  includes_.clear();
  if (thread_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(thread_handle_);
  }
}

void Script::run() {
  try {
    runtime_ = JS_NewRuntime(Vars.dwMemUsage, JS_NO_HELPER_THREADS);
    JS_SetNativeStackQuota(runtime_, (size_t)50000);
    // JS_SetRuntimeThread(runtime);
    JS_SetContextCallback(runtime_, contextCallback);

    context_ = JS_NewContext(runtime_, 0x2000);
    if (!context_) {
      throw std::exception("Couldn't create the context");
    }

    JS_SetErrorReporter(context_, reportError);
    JS_SetOperationCallback(context_, operationCallback);
    JS_SetOptions(context_, JSOPTION_STRICT | JSOPTION_VAROBJFIX);
    JS_SetVersion(context_, JSVERSION_LATEST);
    //

    JS_SetContextPrivate(context_, this);

    JS_BeginRequest(context_);

    globals_ = JS_GetGlobalObject(context_);
    jsval meVal = JSVAL_VOID;
    if (JS_GetProperty(context_, globals_, "me", &meVal) != JS_FALSE) {
      JSObject* meObject = JSVAL_TO_OBJECT(meVal);
      me_ = (myUnit*)JS_GetPrivate(context_, meObject);
    }

    if (type_ == ScriptType::Command) {
      if (wcslen(Vars.szConsole) > 0) {
        script_ = JS_CompileFile(context_, globals_, filename_);
      } else {
        const char* cmd = "function main() {print('ÿc2D2BSÿc0 :: Started Console'); while (true){delay(10000)};}  ";
        script_ = JS_CompileScript(context_, globals_, cmd, strlen(cmd), "Command Line", 1);
      }
      JS_AddNamedScriptRoot(context_, &script_, "console script");
    } else {
      script_ = JS_CompileFile(context_, globals_, filename_);
    }

    if (!script_) {
      throw std::exception("Couldn't compile the script");
    }

    JS_EndRequest(context_);
    // JS_RemoveScriptRoot(context, &script);

  } catch (std::exception&) {
    if (context_) {
      JS_EndRequest(context_);
      JS_DestroyContext(context_);
    }
    throw;
  }
  // only let the script run if it's not already running
  if (is_running()) {
    return;
  }
  hasActiveCX_ = true;
  state_ = ScriptState::Running;

  jsval main = INT_TO_JSVAL(1), dummy = INT_TO_JSVAL(1);
  JS_BeginRequest(context_);

  // args passed from load
  jsval* argvalue = new jsval[argc_];
  for (uint i = 0; i < argc_; i++) {
    argv_[i]->read(context_, &argvalue[i]);
  }

  for (uint j = 0; j < argc_; j++) {
    JS_AddValueRoot(context_, &argvalue[j]);
  }

  JS_AddValueRoot(context_, &main);
  JS_AddValueRoot(context_, &dummy);
  if (JS_ExecuteScript(context_, globals_, script_, &dummy) != JS_FALSE &&
      JS_GetProperty(context_, globals_, "main", &main) != JS_FALSE && JSVAL_IS_FUNCTION(context_, main)) {
    JS_CallFunctionValue(context_, globals_, main, this->argc_, argvalue, &dummy);
  }
  JS_RemoveValueRoot(context_, &main);
  JS_RemoveValueRoot(context_, &dummy);
  for (uint j = 0; j < argc_; j++) {
    JS_RemoveValueRoot(context_, &argvalue[j]);
  }

  /*for(uint i = 0; i < argc; i++)  //crashes spidermonkey cleans itself up?
  {
          argv[i]->clear();
          delete argv[i];
  }*/

  JS_EndRequest(context_);

  execCount_++;
  // Stop();
}

void Script::stop(bool force, bool reallyForce) {
  // if we've already stopped, just return
  if (state_ == ScriptState::Stopped) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  //  tell everyone else that the script is aborted FIRST
  state_ = ScriptState::Stopped;
  if (type_ != ScriptType::Command) {
    const wchar_t* displayName = filename_.c_str() + Vars.script_dir.wstring().length() + 1;
    Print(L"Script %s ended", displayName);
  }

  // trigger call back so script ends
  TriggerOperationCallback();
  SetEvent(event_signal_);

  // normal wait: 500ms, forced wait: 300ms, really forced wait: 100ms
  int maxCount = (force ? (reallyForce ? 10 : 30) : 50);
  if (GetCurrentThreadId() != thread_id()) {
    for (int i = 0; hasActiveCX_ == true; i++) {
      // if we pass the time frame, just ignore the wait because the thread will end forcefully anyway
      if (i >= maxCount) {
        break;
      }
      Sleep(10);
    }
  }
}

void Script::join() {
  std::unique_lock<std::mutex> lock(mutex_);

  HANDLE hThread = thread_handle_;
  if (hThread != INVALID_HANDLE_VALUE) {
    WaitForSingleObject(hThread, INFINITE);
  }
}

void Script::pause() {
  if (!is_stopped() && !is_paused()) {
    state_ = ScriptState::Paused;
  }
  TriggerOperationCallback();
  SetEvent(event_signal_);
}

void Script::resume() {
  if (!is_stopped() && is_paused()) {
    state_ = ScriptState::Running;
  }
  TriggerOperationCallback();
  SetEvent(event_signal_);
}

bool Script::is_running() {
  return context_ && !(is_stopped() || is_paused() || !hasActiveCX_);
}

bool Script::is_paused() {
  return state_ == ScriptState::Paused;
}

bool Script::is_stopped() {
  return state_ == ScriptState::Stopped;
}

bool Script::BeginThread(LPTHREAD_START_ROUTINE ThreadFunc) {
  std::unique_lock<std::mutex> lock(mutex_);

  DWORD dwExitCode = STILL_ACTIVE;

  if ((!GetExitCodeThread(thread_handle_, &dwExitCode) || dwExitCode != STILL_ACTIVE) &&
      (thread_handle_ = CreateThread(0, 0, ThreadFunc, this, 0, &thread_id_)) != NULL) {
#ifdef DEBUG
    SetThreadDescription(thread_handle_, filename_short());
#endif
    return true;
  }

  thread_handle_ = INVALID_HANDLE_VALUE;
  return false;
}

void Script::RunCommand(const wchar_t* command) {
  // RUNCOMMANDSTRUCT* rcs = new RUNCOMMANDSTRUCT;
  // rcs->script = this;
  // rcs->command = _wcsdup(command);

  if (state_ == ScriptState::Stopped) {  // this should never happen -bob
    // RUNCOMMANDSTRUCT* rcs = new RUNCOMMANDSTRUCT;

    // rcs->script = this;
    // rcs->command = _wcsdup(L"delay(1000000);");

    Log(L"Console Aborted HELP!");
    // HANDLE hwnd = CreateThread(NULL, 0, RunCommandThread, (void*) rcs, 0, NULL);
  }

  Event* evt = new Event;
  evt->owner = this;
  evt->argc = argc_;
  evt->name = _strdup("Command");
  evt->arg1 = _wcsdup(command);

  evt->owner->FireEvent(evt);
}

const wchar_t* Script::filename_short() {
  if (wcscmp(filename_.c_str(), L"Command Line") == 0)
    return filename_.c_str();
  else
    return (filename_.c_str() + Vars.script_dir.wstring().length() + 1);
}

int Script::GetExecutionCount() {
  return execCount_;
}

void Script::UpdatePlayerGid() {
  me_->dwUnitId = (D2CLIENT_GetPlayerUnit() == NULL ? NULL : D2CLIENT_GetPlayerUnit()->dwUnitId);
}

bool Script::IsIncluded(const wchar_t* file) {
  uint count = 0;
  wchar_t* fname = _wcsdup(file);
  if (!fname) return false;

  _wcslwr_s(fname, wcslen(fname) + 1);
  StringReplace(fname, '/', '\\', wcslen(fname));
  count = includes_.count(std::wstring(fname));
  free(fname);

  return !!count;
}

bool Script::Include(const wchar_t* file) {
  // THREAD SAFETY! ~ ejt
  // Removed critical section lock here because Include is called recursively, one included script file can include more
  // script files which causes a dead-lock. If any thread synchronization issues occurs here it is necessary to rethink
  // a solution on how to deal with that.

  wchar_t* fname = _wcsdup(file);
  if (!fname) return false;
  _wcslwr_s(fname, wcslen(fname) + 1);
  StringReplace(fname, L'/', L'\\', wcslen(fname));

  // don't invoke the string ctor more than once...
  std::wstring currentFileName = std::wstring(fname);
  // ignore already included, 'in-progress' includes, and self-inclusion
  if (!!includes_.count(fname) || !!inProgress_.count(fname) || (currentFileName.compare(filename_.c_str()) == 0)) {
    free(fname);
    return true;
  }
  bool rval = false;

  JSContext* cx = context_;

  JS_BeginRequest(cx);

  JSScript* _script = JS_CompileFile(cx, globals_, fname);
  if (_script) {
    jsval dummy;
    inProgress_[fname] = true;
    rval = !!JS_ExecuteScript(cx, globals_, _script, &dummy);
    if (rval) {
      includes_[fname] = true;
    } else {
      JS_ReportPendingException(cx);
    }
    inProgress_.erase(fname);
  } else {
    JS_ReportPendingException(cx);
  }

  JS_EndRequest(cx);
  free(fname);
  return rval;
}

bool Script::IsListenerRegistered(const char* evtName) {
  return strlen(evtName) > 0 && functions_.count(evtName) > 0;
}

void Script::RegisterEvent(const char* evtName, jsval evtFunc) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (JSVAL_IS_FUNCTION(context_, evtFunc) && strlen(evtName) > 0) {
    AutoRoot* root = new AutoRoot(context_, evtFunc);
    functions_[evtName].push_back(root);
  }
}

bool Script::IsRegisteredEvent(const char* evtName, jsval evtFunc) {
  if (strlen(evtName) < 1 || !functions_.contains(evtName)) {
    return false;
  }

  for (auto* root : functions_[evtName]) {
    if (*(root->value()) == evtFunc) {
      return true;
    }
  }

  return false;
}

void Script::UnregisterEvent(const char* evtName, jsval evtFunc) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (strlen(evtName) < 1 || !functions_.contains(evtName)) {
    return;
  }

  AutoRoot* func = NULL;
  for (FunctionList::iterator it = functions_[evtName].begin(); it != functions_[evtName].end(); it++) {
    if (*(*it)->value() == evtFunc) {
      func = *it;
      break;
    }
  }
  functions_[evtName].remove(func);
  // func->Release();
  delete func;

  // Remove event completely if there are no listeners for it.
  if (functions_.count(evtName) > 0 && functions_[evtName].size() == 0) {
    functions_.erase(evtName);
  }
}

void Script::ClearEvent(const char* evtName) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (FunctionList::iterator it = functions_[evtName].begin(); it != functions_[evtName].end(); it++) {
    AutoRoot* func = *it;
    func->Release();
    delete func;
  }
  functions_[evtName].clear();
}

void Script::ClearAllEvents() {
  std::lock_guard<std::mutex> lock(mutex_);

  for (FunctionMap::iterator it = functions_.begin(); it != functions_.end(); it++) ClearEvent(it->first.c_str());
  functions_.clear();
}

void Script::FireEvent(Event* evt) {
  event_queue_.enqueue(std::move(evt));

  EnterCriticalSection(&Vars.cEventSection);
  evt->owner->events().push_front(evt);
  LeaveCriticalSection(&Vars.cEventSection);

  if (evt->owner && evt->owner->is_running()) {
    evt->owner->TriggerOperationCallback();
  }
  SetEvent(event_signal_);
}

void Script::process_events() {
  Event* evt = nullptr;

  while (event_queue_.dequeue_for(evt, std::chrono::milliseconds(10))) {
    auto name = AnsiToUnicode(evt->name);
    Print(L"Process event: %s", name);
    delete[] name;
  }
}

#ifdef DEBUG
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;      // must be 0x1000
  LPCWSTR szName;    // pointer to name (in user addr space)
  DWORD dwThreadID;  // thread ID (-1=caller thread)
  DWORD dwFlags;     // reserved for future use, must be zero
} THREADNAME_INFO;

void SetThreadName(DWORD dwThreadID, LPCWSTR szThreadName) {
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = szThreadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try {
    RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
  } __except (EXCEPTION_CONTINUE_EXECUTION) {
  }
}
#endif

DWORD WINAPI RunCommandThread(void* data) {
  RUNCOMMANDSTRUCT* rcs = (RUNCOMMANDSTRUCT*)data;
  JSContext* cx = rcs->script->context();
  JS_BeginRequest(cx);
  jsval rval;
  JS_AddNamedValueRoot(cx, &rval, "Cmd line rtl");
  if (JS_EvaluateUCScript(cx, JS_GetGlobalObject(cx), rcs->command, wcslen(rcs->command), "Command Line", 0, &rval)) {
    if (!JSVAL_IS_NULL(rval) && !JSVAL_IS_VOID(rval)) {
      JS_ConvertValue(cx, rval, JSTYPE_STRING, &rval);
      const wchar_t* text = JS_GetStringCharsZ(cx, JS_ValueToString(cx, rval));
      Print(L"%s", text);
    }
  }
  JS_RemoveRoot(cx, &rval);
  JS_EndRequest(cx);
  JS_TriggerOperationCallback(JS_GetRuntime(cx));
  return 0;
}

DWORD WINAPI ScriptThread(void* data) {
  Script* script = (Script*)data;
  if (script) {
#ifdef DEBUG
    SetThreadName(0xFFFFFFFF, script->filename_short());
#endif
    script->run();
    if (Vars.bDisableCache) {
      script->engine()->DisposeScript(script);
    }
  }
  return 0;
}

bool ExecScriptEvent(Event* evt, bool clearList) {
  JSContext* cx = nullptr;

  if (!clearList) {
    cx = evt->owner->context();
  }

  char* evtName = (char*)evt->name;
  if (strcmp(evtName, "itemaction") == 0) {
    if (!clearList) {
      DWORD* gid = (DWORD*)evt->arg1;
      char* code = (char*)evt->arg2;
      DWORD* mode = (DWORD*)evt->arg3;
      bool* global = (bool*)evt->arg4;

      jsval* argv = new jsval[evt->argc];
      JS_BeginRequest(cx);

      argv[0] = JS_NumberValue(*gid);
      argv[1] = JS_NumberValue(*mode);
      argv[2] = (STRING_TO_JSVAL(JS_NewStringCopyZ(cx, code)));
      argv[3] = (BOOLEAN_TO_JSVAL(*global));
      for (int j = 0; j < 4; j++) {
        JS_AddValueRoot(cx, &argv[j]);
      }

      jsval rval;
      bool block = false;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 4, argv, &rval);
        block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
      }

      JS_EndRequest(cx);

      for (int j = 0; j < 4; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }

    delete evt->arg1;
    free(evt->arg2);
    delete evt->arg3;
    delete evt->arg4;
    free(evt->name);
    delete evt;
    return true;
  }
  if (strcmp(evtName, "gameevent") == 0) {
    if (!clearList) {
      jsval* argv = new jsval[5];
      JS_BeginRequest(cx);
      argv[0] = JS_NumberValue(*(BYTE*)evt->arg1);
      argv[1] = JS_NumberValue(*(DWORD*)evt->arg2);
      argv[2] = JS_NumberValue(*(DWORD*)evt->arg3);
      argv[3] = (STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (char*)evt->arg4)));
      argv[4] = (STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (wchar_t*)evt->arg5)));

      for (int j = 0; j < 5; j++) {
        JS_AddValueRoot(cx, &argv[j]);
      }

      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 5, argv, &rval);
      }
      JS_EndRequest(cx);
      for (int j = 0; j < 5; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }

    delete evt->arg1;
    delete evt->arg2;
    delete evt->arg3;
    free(evt->arg4);
    free(evt->arg5);
    free(evt->name);
    delete evt;
    return true;
  }
  if (strcmp(evtName, "copydata") == 0) {
    if (!clearList) {
      jsval* argv = new jsval[2];
      JS_BeginRequest(cx);
      argv[0] = JS_NumberValue(*(DWORD*)evt->arg1);
      argv[1] = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (wchar_t*)evt->arg2));

      for (int j = 0; j < 2; j++) JS_AddValueRoot(cx, &argv[j]);

      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2, argv, &rval);
      }
      JS_EndRequest(cx);
      for (int j = 0; j < 2; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }
    delete evt->arg1;
    free(evt->arg2);
    free(evt->name);
    delete evt;
    return true;
  }
  if (strcmp(evtName, "chatmsg") == 0 || strcmp(evtName, "chatinput") == 0 || strcmp(evtName, "whispermsg") == 0 ||
      strcmp(evtName, "chatmsgblocker") == 0 || strcmp(evtName, "chatinputblocker") == 0 ||
      strcmp(evtName, "whispermsgblocker") == 0) {
    bool block = false;
    if (!clearList) {
      jsval* argv = new jsval[2];
      JS_BeginRequest(cx);
      argv[0] = (STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (char*)evt->arg1)));
      argv[1] = (STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (wchar_t*)evt->arg2)));

      for (int j = 0; j < 2; j++) {
        JS_AddValueRoot(cx, &argv[j]);
      }

      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2, argv, &rval);
        block |= (JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
      }
      JS_EndRequest(cx);
      for (int j = 0; j < 2; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }
    if (strcmp(evtName, "chatmsgblocker") == 0 || strcmp(evtName, "chatinputblocker") == 0 ||
        strcmp(evtName, "whispermsgblocker") == 0) {
      *(DWORD*)evt->arg4 = block;
      SetEvent(Vars.eventSignal);
    } else {
      free(evt->arg1);
      free(evt->arg2);
      free(evt->name);
      delete evt;
    }
    return true;
  }
  if (strcmp(evtName, "mousemove") == 0 || strcmp(evtName, "ScreenHookHover") == 0) {
    if (!clearList) {
      jsval* argv = new jsval[2];
      JS_BeginRequest(cx);
      argv[0] = JS_NumberValue(*(DWORD*)evt->arg1);
      argv[1] = JS_NumberValue(*(DWORD*)evt->arg2);

      for (int j = 0; j < 2; j++) {
        JS_AddValueRoot(cx, &argv[j]);
      }

      jsval rval;
      if (strcmp(evtName, "ScreenHookHover") == 0) {
        for (FunctionList::iterator it = evt->functions.begin(); it != evt->functions.end(); it++)
          JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), evt->argc + 1, argv, &rval);
      } else {
        for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
             it != evt->owner->functions()[evtName].end(); it++)
          JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 2, argv, &rval);
      }
      JS_EndRequest(cx);
      for (int j = 0; j < 2; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }
    delete evt->arg1;
    delete evt->arg2;
    free(evt->name);
    delete evt;
    return true;
  }
  if (strcmp(evtName, "mouseclick") == 0) {
    if (!clearList) {
      jsval* argv = new jsval[4];
      JS_BeginRequest(cx);
      argv[0] = JS_NumberValue(*(DWORD*)evt->arg1);
      argv[1] = JS_NumberValue(*(DWORD*)evt->arg2);
      argv[2] = JS_NumberValue(*(DWORD*)evt->arg3);
      argv[3] = JS_NumberValue(*(DWORD*)evt->arg4);

      for (uint j = 0; j < evt->argc; j++) JS_AddValueRoot(cx, &argv[j]);

      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 4, argv, &rval);
      }
      JS_EndRequest(cx);
      for (int j = 0; j < 4; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }
    delete evt->arg1;
    delete evt->arg2;
    delete evt->arg3;
    delete evt->arg4;
    free(evt->name);
    delete evt;
    return true;
  }
  if (strcmp(evtName, "keyup") == 0 || strcmp(evtName, "keydownblocker") == 0 || strcmp(evtName, "keydown") == 0 ||
      strcmp(evtName, "memana") == 0 || strcmp(evtName, "melife") == 0 || strcmp(evtName, "playerassign") == 0) {
    bool block = false;
    if (!clearList) {
      jsval* argv = new jsval[1];
      JS_BeginRequest(cx);
      argv[0] = JS_NumberValue(*(DWORD*)evt->arg1);
      JS_AddValueRoot(cx, &argv[0]);
      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, argv, &rval);
        block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
      }
      JS_EndRequest(cx);
      JS_RemoveValueRoot(cx, &argv[0]);
    }
    if (strcmp(evtName, "keydownblocker") == 0) {
      *(DWORD*)evt->arg4 = block;
      SetEvent(Vars.eventSignal);
    } else {
      delete evt->arg1;
      free(evt->name);
      delete evt;
    }
    return true;
  }
  if (strcmp(evtName, "ScreenHookClick") == 0) {
    bool block = false;
    if (!clearList) {
      jsval* argv = new jsval[3];
      JS_BeginRequest(cx);
      argv[0] = JS_NumberValue(*(DWORD*)evt->arg1);
      argv[1] = JS_NumberValue(*(DWORD*)evt->arg2);
      argv[2] = JS_NumberValue(*(DWORD*)evt->arg3);
      for (int j = 0; j < 3; j++) {
        JS_AddValueRoot(cx, &argv[j]);
      }

      jsval rval;
      // diffrent function source for hooks
      for (FunctionList::iterator it = evt->functions.begin(); it != evt->functions.end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 3, argv, &rval);
        block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
      }

      JS_EndRequest(cx);
      for (int j = 0; j < 3; j++) JS_RemoveValueRoot(cx, &argv[j]);
    }
    *(DWORD*)evt->arg4 = block;
    SetEvent(Vars.eventSignal);

    return true;
  }
  if (strcmp(evtName, "Command") == 0) {
    wchar_t* cmd = (wchar_t*)evt->arg1;
    std::wstring test;

    test.append(L"try{ ");
    test.append(cmd);
    test.append(L" } catch (error){print(error)}");
    JS_BeginRequest(cx);
    jsval rval;

    if (JS_EvaluateUCScript(cx, JS_GetGlobalObject(cx), test.data(), test.length(), "Command Line", 0, &rval)) {
      if (!JSVAL_IS_NULL(rval) && !JSVAL_IS_VOID(rval)) {
        JS_ConvertValue(cx, rval, JSTYPE_STRING, &rval);
        const wchar_t* text = JS_GetStringCharsZ(cx, JS_ValueToString(cx, rval));
        Print(L"%s", text);
      }
    }
    JS_EndRequest(cx);
    free(evt->arg1);
    free(evt->name);
    delete evt;
  }
  if (strcmp(evtName, "scriptmsg") == 0) {
    if (!clearList) {
      DWORD* argc = (DWORD*)evt->arg1;
      JS_BeginRequest(cx);
      jsval* argv = new jsval[*argc];
      for (uint i = 0; i < *argc; i++) {
        evt->argv[i]->read(cx, &argv[i]);
      }

      for (uint j = 0; j < *argc; j++) {
        JS_AddValueRoot(cx, &argv[j]);
      }

      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), *argc, argv, &rval);
      }
      JS_EndRequest(cx);

      for (uint j = 0; j < *argc; j++) {
        JS_RemoveValueRoot(cx, &argv[j]);
      }
    }
    for (uint i = 0; i < evt->argc; i++) {
      evt->argv[i]->clear();
      delete evt->argv[i];
    }
    delete evt->arg1;
    free(evt->name);
    delete evt;
    return true;
  }
  if (strcmp(evtName, "gamepacket") == 0 || strcmp(evtName, "gamepacketsent") == 0 ||
      strcmp(evtName, "realmpacket") == 0) {
    bool block = false;
    if (!clearList) {
      BYTE* help = (BYTE*)evt->arg1;
      DWORD* size = (DWORD*)evt->arg2;
      //  DWORD* argc = (DWORD*)1;
      JS_BeginRequest(cx);

      JSObject* arr = JS_NewUint8Array(cx, *size);
      // JSObject* arr = JS_NewArrayObject(cx, 0, NULL);

      JS_AddRoot(cx, &arr);
      for (uint i = 0; i < *size; i++) {
        jsval jsarr = UINT_TO_JSVAL(help[i]);
        JS_SetElement(cx, arr, i, &jsarr);
      }
      jsval argv = OBJECT_TO_JSVAL(arr);
      // evt->argv[0]->read(cx, &argv[0]);
      // JS_AddValueRoot(cx, &argv[0]);

      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 1, &argv, &rval);
        block |= static_cast<bool>(JSVAL_IS_BOOLEAN(rval) && JSVAL_TO_BOOLEAN(rval));
      }
      *(DWORD*)evt->arg4 = block;
      JS_RemoveRoot(cx, &arr);
      SetEvent(Vars.eventSignal);
      JS_EndRequest(cx);
      //	for(int j = 0 ; j < *argc; j++)
      //	JS_RemoveValueRoot(cx, &argv[j]);
    }

    // delete evt->arg1;
    // delete evt->arg2;

    return true;
  }
  if (strcmp(evtName, "setTimeout") == 0 || strcmp(evtName, "setInterval") == 0) {
    if (!clearList) {
      JS_BeginRequest(cx);
      jsval rval;
      for (FunctionList::iterator it = evt->owner->functions()[evtName].begin();
           it != evt->owner->functions()[evtName].end(); it++) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(*it)->value(), 0, &rval, &rval);
      }
      JS_EndRequest(cx);
    }

    if (strcmp(evtName, "setTimeout") == 0) {
      evt->owner->engine()->RemoveDelayedEvent(*(DWORD*)evt->arg1);
    }

    return true;
  }
  if (strcmp(evtName, "DisposeMe") == 0) {
    evt->owner->engine()->DisposeScript(evt->owner);
  }

  return true;
}

JSBool operationCallback(JSContext* cx) {
  Script* script = (Script*)JS_GetContextPrivate(cx);
  static int callBackCount = 0;
  callBackCount++;

  while (script->is_paused()) {
    Sleep(50);
    JS_MaybeGC(cx);
  }

  if (!!!(JSBool)(script->is_stopped() ||
                  ((script->type() == ScriptType::InGame) && ClientState() == ClientStateMenu))) {
    // TEMPORARY: Still to much to detangle from the current event system to figure out where to put this call
    script->process_events();

    auto& events = script->events();
    while (events.size() > 0 && !!!(JSBool)(script->is_stopped() || ((script->type() == ScriptType::InGame) &&
                                                                     ClientState() == ClientStateMenu))) {
      EnterCriticalSection(&Vars.cEventSection);
      Event* evt = events.back();
      events.pop_back();
      LeaveCriticalSection(&Vars.cEventSection);
      ExecScriptEvent(evt, false);
    }
    return !!!(JSBool)(script->is_stopped() ||
                       ((script->type() == ScriptType::InGame) && ClientState() == ClientStateMenu));
  } else {
    return false;
  }
}

JSBool contextCallback(JSContext* cx, uint contextOp) {
  switch (contextOp) {
    case JSCONTEXT_NEW: {
      JS_BeginRequest(cx);

      JS_SetErrorReporter(cx, reportError);
      JS_SetOperationCallback(cx, operationCallback);

      JS_SetVersion(cx, JSVERSION_LATEST);
      JS_SetOptions(cx, JSOPTION_METHODJIT | JSOPTION_TYPE_INFERENCE | JSOPTION_ION | JSOPTION_VAROBJFIX |
                            JSOPTION_ASMJS | JSOPTION_STRICT);

      // JS_SetGCZeal(cx, 2, 1);
      JSObject* globalObject = JS_NewGlobalObject(cx, &global_obj, NULL);
      JS_SetGCParameter(JS_GetRuntime(cx), JSGC_MODE, 2);

      if (JS_InitStandardClasses(cx, globalObject) == JS_FALSE) {
        return JS_FALSE;
      }

      if (JS_DefineFunctions(cx, globalObject, global_funcs) == JS_FALSE) {
        return JS_FALSE;
      }

      myUnit* lpUnit = new myUnit;
      memset(lpUnit, NULL, sizeof(myUnit));

      UnitAny* player = D2CLIENT_GetPlayerUnit();
      lpUnit->dwMode = (DWORD)-1;
      lpUnit->dwClassId = (DWORD)-1;
      lpUnit->dwType = UNIT_PLAYER;
      lpUnit->dwUnitId = player ? player->dwUnitId : NULL;
      lpUnit->_dwPrivateType = PRIVATE_UNIT;

      for (JSClassSpec* entry = global_classes; entry->classp != NULL; entry++) {
        if (!JS_InitClass(cx, globalObject, NULL, entry->classp, entry->classp->construct, 0, entry->properties,
                          entry->methods, entry->static_properties, entry->static_methods)) {
          throw std::exception("Couldn't initialize the class");
        }
      }

      JSObject* meObject = BuildObject(cx, &unit_class, unit_methods, me_props, lpUnit);
      if (!meObject) return JS_FALSE;

      if (JS_DefineProperty(cx, globalObject, "me", OBJECT_TO_JSVAL(meObject), NULL, NULL, JSPROP_PERMANENT_VAR) ==
          JS_FALSE) {
        return JS_FALSE;
      }

#define DEFCONST(vp)                                                                                   \
  if (!JS_DefineProperty(cx, globalObject, #vp, INT_TO_JSVAL(vp), NULL, NULL, JSPROP_PERMANENT_VAR)) { \
    throw std::exception("Couldn't initialize the constant");                                          \
  }
      DEFCONST(FILE_READ);
      DEFCONST(FILE_WRITE);
      DEFCONST(FILE_APPEND);
#undef DEFCONST

      JS_EndRequest(cx);
    } break;

    case JSCONTEXT_DESTROY: {
      Script* script = (Script*)JS_GetContextPrivate(cx);
      script->set_has_active_cx(false);
      // TEMPORARY: Still to much to detangle from the current event system to figure out where to put this call
      script->process_events();

      auto& events = script->events();
      while (events.size() > 0) {
        EnterCriticalSection(&Vars.cEventSection);
        Event* evt = events.back();
        events.pop_back();
        LeaveCriticalSection(&Vars.cEventSection);
        ExecScriptEvent(evt, true);  // clean list and pop events
      }

      script->ClearAllEvents();
      Genhook::Clean(script);
    } break;
  }
  return JS_TRUE;
}
