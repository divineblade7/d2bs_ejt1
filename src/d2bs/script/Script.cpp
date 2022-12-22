#include "d2bs/script/Script.h"

#include "d2bs/D2BS.h"
#include "d2bs/core/Core.h"
#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/utils/Helpers.h"

#include <algorithm>
#include <io.h>

Script::Script(const wchar_t* file, ScriptType type, uint argc, JSAutoStructuredCloneBuffer** argv)
    : type_(type), argv_(argv), argc_(argc) {
  InitializeCriticalSection(&lock_);

  eventSignal_ = CreateEvent(nullptr, true, false, nullptr);

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
  // JS_SetPendingException(context, JSVAL_NULL);
  if (JS_IsInRequest(runtime_)) {
    JS_EndRequest(context_);
  }

  EnterCriticalSection(&lock_);
  //    JS_SetRuntimeThread(rt);
  JS_DestroyContext(context_);
  // JS_ClearRuntimeThread(rt);
  JS_DestroyRuntime(runtime_);

  context_ = nullptr;
  globals_ = nullptr;
  script_ = nullptr;
  CloseHandle(eventSignal_);
  includes_.clear();
  if (thread_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(thread_handle_);
  }
  LeaveCriticalSection(&lock_);
  DeleteCriticalSection(&lock_);
}

void Script::Run() {
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
    LeaveCriticalSection(&lock_);
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

void Script::Join() {
  EnterCriticalSection(&lock_);
  HANDLE hThread = thread_handle_;
  LeaveCriticalSection(&lock_);

  if (hThread != INVALID_HANDLE_VALUE) {
    WaitForSingleObject(hThread, INFINITE);
  }
}

void Script::Pause() {
  if (!is_stopped() && !is_paused()) {
    state_ = ScriptState::Paused;
  }
  TriggerOperationCallback();
  SetEvent(eventSignal_);
}

void Script::Resume() {
  if (!is_stopped() && is_paused()) {
    state_ = ScriptState::Running;
  }
  TriggerOperationCallback();
  SetEvent(eventSignal_);
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
  EnterCriticalSection(&lock_);
  DWORD dwExitCode = STILL_ACTIVE;

  if ((!GetExitCodeThread(thread_handle_, &dwExitCode) || dwExitCode != STILL_ACTIVE) &&
      (thread_handle_ = CreateThread(0, 0, ThreadFunc, this, 0, &thread_id_)) != NULL) {
    LeaveCriticalSection(&lock_);
    return true;
  }

  thread_handle_ = INVALID_HANDLE_VALUE;
  LeaveCriticalSection(&lock_);
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
  EnterCriticalSection(&Vars.cEventSection);
  evt->owner->events().push_front(evt);
  LeaveCriticalSection(&Vars.cEventSection);
  evt->owner->TriggerOperationCallback();
  SetEvent(evt->owner->eventSignal_);
}

void Script::Stop(bool force, bool reallyForce) {
  // if we've already stopped, just return
  if (state_ == ScriptState::Stopped) {
    return;
  }
  EnterCriticalSection(&lock_);
  // tell everyone else that the script is aborted FIRST
  state_ = ScriptState::Stopped;
  if (type_ != ScriptType::Command) {
    const wchar_t* displayName = filename_.c_str() + wcslen(Vars.szScriptPath) + 1;
    Print(L"Script %s ended", displayName);
  }

  // trigger call back so script ends
  TriggerOperationCallback();
  SetEvent(eventSignal_);

  // normal wait: 500ms, forced wait: 300ms, really forced wait: 100ms
  int maxCount = (force ? (reallyForce ? 10 : 30) : 50);
  if (GetCurrentThreadId() != thread_id()) {
    for (int i = 0; hasActiveCX_ == true; i++) {
      // if we pass the time frame, just ignore the wait because the thread will end forcefully anyway
      if (i >= maxCount) break;
      Sleep(10);
    }
  }
  LeaveCriticalSection(&lock_);
}

const wchar_t* Script::filename_short() {
  if (wcscmp(filename_.c_str(), L"Command Line") == 0)
    return filename_.c_str();
  else
    return (filename_.c_str() + wcslen(Vars.szScriptPath) + 1);
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
  // since includes will happen on the same thread, locking here is acceptable
  EnterCriticalSection(&lock_);
  wchar_t* fname = _wcsdup(file);
  if (!fname) return false;
  _wcslwr_s(fname, wcslen(fname) + 1);
  StringReplace(fname, L'/', L'\\', wcslen(fname));

  // don't invoke the string ctor more than once...
  std::wstring currentFileName = std::wstring(fname);
  // ignore already included, 'in-progress' includes, and self-inclusion
  if (!!includes_.count(fname) || !!inProgress_.count(fname) || (currentFileName.compare(filename_.c_str()) == 0)) {
    LeaveCriticalSection(&lock_);
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
    // JS_RemoveRoot(&scriptObj);
  } else {
    JS_ReportPendingException(cx);
  }

  JS_EndRequest(cx);
  // JS_RemoveScriptRoot(cx, &script);
  LeaveCriticalSection(&lock_);
  free(fname);
  return rval;
}

bool Script::IsListenerRegistered(const char* evtName) {
  return strlen(evtName) > 0 && functions_.count(evtName) > 0;
}

void Script::RegisterEvent(const char* evtName, jsval evtFunc) {
  EnterCriticalSection(&lock_);
  if (JSVAL_IS_FUNCTION(context_, evtFunc) && strlen(evtName) > 0) {
    AutoRoot* root = new AutoRoot(context_, evtFunc);
    functions_[evtName].push_back(root);
  }
  LeaveCriticalSection(&lock_);
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
  EnterCriticalSection(&lock_);

  if (strlen(evtName) < 1 || !functions_.contains(evtName)) {
    LeaveCriticalSection(&lock_);
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

  LeaveCriticalSection(&lock_);
}

void Script::ClearEvent(const char* evtName) {
  EnterCriticalSection(&lock_);
  for (FunctionList::iterator it = functions_[evtName].begin(); it != functions_[evtName].end(); it++) {
    AutoRoot* func = *it;
    func->Release();
    delete func;
  }
  functions_[evtName].clear();
  LeaveCriticalSection(&lock_);
}

void Script::ClearAllEvents() {
  EnterCriticalSection(&lock_);
  for (FunctionMap::iterator it = functions_.begin(); it != functions_.end(); it++) ClearEvent(it->first.c_str());
  functions_.clear();
  LeaveCriticalSection(&lock_);
}

void Script::FireEvent(Event* evt) {
  EnterCriticalSection(&Vars.cEventSection);
  evt->owner->events().push_front(evt);
  LeaveCriticalSection(&Vars.cEventSection);

  if (evt->owner && evt->owner->is_running()) {
    evt->owner->TriggerOperationCallback();
  }
  SetEvent(eventSignal_);
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
    script->Run();
    if (Vars.bDisableCache) {
      sScriptEngine->DisposeScript(script);
    }
  }
  return 0;
}
