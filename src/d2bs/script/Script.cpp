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

Script::Script(const wchar_t* file, ScriptState state, uint argc, JSAutoStructuredCloneBuffer** argv)
    : context_(NULL),
      globalObject_(NULL),
      scriptObject_(NULL),
      script_(NULL),
      execCount_(0),
      isAborted_(false),
      isPaused_(false),
      isReallyPaused_(false),
      scriptState_(state),
      threadHandle_(INVALID_HANDLE_VALUE),
      threadId_(0),
      argc_(argc),
      argv_(argv) {
  InitializeCriticalSection(&lock_);
  // moved the runtime initilization to thread start
  LastGC_ = 0;
  hasActiveCX_ = false;
  eventSignal_ = CreateEvent(nullptr, true, false, nullptr);

  if (scriptState_ == Command && wcslen(file) < 1) {
    fileName_ = std::wstring(L"Command Line");
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
    fileName_ = std::wstring(tmpName);
    replace(fileName_.begin(), fileName_.end(), L'/', L'\\');
    free(tmpName);
  }
}

Script::~Script(void) {
  isAborted_ = true;
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

  context_ = NULL;
  scriptObject_ = NULL;
  globalObject_ = NULL;
  script_ = NULL;
  CloseHandle(eventSignal_);
  includes_.clear();
  if (threadHandle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(threadHandle_);
  }
  LeaveCriticalSection(&lock_);
  DeleteCriticalSection(&lock_);
}

void Script::Run(void) {
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

    globalObject_ = JS_GetGlobalObject(context_);
    jsval meVal = JSVAL_VOID;
    if (JS_GetProperty(GetContext(), globalObject_, "me", &meVal) != JS_FALSE) {
      JSObject* meObject = JSVAL_TO_OBJECT(meVal);
      me_ = (myUnit*)JS_GetPrivate(GetContext(), meObject);
    }

    if (scriptState_ == Command) {
      if (wcslen(Vars.szConsole) > 0) {
        script_ = JS_CompileFile(context_, globalObject_, fileName_);
      } else {
        const char* cmd = "function main() {print('ÿc2D2BSÿc0 :: Started Console'); while (true){delay(10000)};}  ";
        script_ = JS_CompileScript(context_, globalObject_, cmd, strlen(cmd), "Command Line", 1);
      }
      JS_AddNamedScriptRoot(context_, &script_, "console script");
    } else {
      script_ = JS_CompileFile(context_, globalObject_, fileName_);
    }

    if (!script_) {
      throw std::exception("Couldn't compile the script");
    }

    JS_EndRequest(context_);
    // JS_RemoveScriptRoot(context, &script);

  } catch (std::exception&) {
    if (scriptObject_) JS_RemoveRoot(context_, &scriptObject_);
    if (context_) {
      JS_EndRequest(context_);
      JS_DestroyContext(context_);
    }
    LeaveCriticalSection(&lock_);
    throw;
  }
  // only let the script run if it's not already running
  if (IsRunning()) return;
  hasActiveCX_ = true;
  isAborted_ = false;

  jsval main = INT_TO_JSVAL(1), dummy = INT_TO_JSVAL(1);
  JS_BeginRequest(GetContext());

  // args passed from load
  jsval* argvalue = new jsval[argc_];
  for (uint i = 0; i < argc_; i++) {
    argv_[i]->read(context_, &argvalue[i]);
  }

  for (uint j = 0; j < argc_; j++) {
    JS_AddValueRoot(context_, &argvalue[j]);
  }

  JS_AddValueRoot(GetContext(), &main);
  JS_AddValueRoot(GetContext(), &dummy);
  if (JS_ExecuteScript(GetContext(), globalObject_, script_, &dummy) != JS_FALSE &&
      JS_GetProperty(GetContext(), globalObject_, "main", &main) != JS_FALSE && JSVAL_IS_FUNCTION(GetContext(), main)) {
    JS_CallFunctionValue(GetContext(), globalObject_, main, this->argc_, argvalue, &dummy);
  }
  JS_RemoveValueRoot(GetContext(), &main);
  JS_RemoveValueRoot(GetContext(), &dummy);
  for (uint j = 0; j < argc_; j++) {
    JS_RemoveValueRoot(GetContext(), &argvalue[j]);
  }

  /*for(uint i = 0; i < argc; i++)  //crashes spidermonkey cleans itself up?
  {
          argv[i]->clear();
          delete argv[i];
  }*/

  JS_EndRequest(GetContext());

  execCount_++;
  // Stop();
}

void Script::Join() {
  EnterCriticalSection(&lock_);
  HANDLE hThread = threadHandle_;
  LeaveCriticalSection(&lock_);

  if (hThread != INVALID_HANDLE_VALUE) {
    WaitForSingleObject(hThread, INFINITE);
  }
}

void Script::Pause(void) {
  if (!IsAborted() && !IsPaused()) {
    isPaused_ = true;
  }
  TriggerOperationCallback();
  SetEvent(eventSignal_);
}

void Script::Resume(void) {
  if (!IsAborted() && IsPaused()) {
    isPaused_ = false;
  }
  TriggerOperationCallback();
  SetEvent(eventSignal_);
}

bool Script::IsPaused(void) {
  return isPaused_;
}

bool Script::BeginThread(LPTHREAD_START_ROUTINE ThreadFunc) {
  EnterCriticalSection(&lock_);
  DWORD dwExitCode = STILL_ACTIVE;

  if ((!GetExitCodeThread(threadHandle_, &dwExitCode) || dwExitCode != STILL_ACTIVE) &&
      (threadHandle_ = CreateThread(0, 0, ThreadFunc, this, 0, &threadId_)) != NULL) {
    LeaveCriticalSection(&lock_);
    return true;
  }

  threadHandle_ = INVALID_HANDLE_VALUE;
  LeaveCriticalSection(&lock_);
  return false;
}

void Script::RunCommand(const wchar_t* command) {
  // RUNCOMMANDSTRUCT* rcs = new RUNCOMMANDSTRUCT;
  // rcs->script = this;
  // rcs->command = _wcsdup(command);

  if (isAborted_) {  // this should never happen -bob
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
  if (isAborted_) {
    return;
  }
  EnterCriticalSection(&lock_);
  // tell everyone else that the script is aborted FIRST
  isAborted_ = true;
  isPaused_ = false;
  isReallyPaused_ = false;
  if (GetState() != Command) {
    const wchar_t* displayName = fileName_.c_str() + wcslen(Vars.szScriptPath) + 1;
    Print(L"Script %s ended", displayName);
  }

  // trigger call back so script ends
  TriggerOperationCallback();
  SetEvent(eventSignal_);

  // normal wait: 500ms, forced wait: 300ms, really forced wait: 100ms
  int maxCount = (force ? (reallyForce ? 10 : 30) : 50);
  if (GetCurrentThreadId() != GetThreadId()) {
    for (int i = 0; hasActiveCX_ == true; i++) {
      // if we pass the time frame, just ignore the wait because the thread will end forcefully anyway
      if (i >= maxCount) break;
      Sleep(10);
    }
  }
  LeaveCriticalSection(&lock_);
}

const wchar_t* Script::GetShortFilename() {
  if (wcscmp(fileName_.c_str(), L"Command Line") == 0)
    return fileName_.c_str();
  else
    return (fileName_.c_str() + wcslen(Vars.szScriptPath) + 1);
}

int Script::GetExecutionCount(void) {
  return execCount_;
}

DWORD Script::GetThreadId(void) {
  return (threadHandle_ == INVALID_HANDLE_VALUE ? -1 : threadId_);
}

void Script::UpdatePlayerGid(void) {
  me_->dwUnitId = (D2CLIENT_GetPlayerUnit() == NULL ? NULL : D2CLIENT_GetPlayerUnit()->dwUnitId);
}

bool Script::IsRunning(void) {
  return context_ && !(IsAborted() || IsPaused() || !hasActiveCX_);
}

bool Script::IsAborted() {
  return isAborted_;
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
  if (!!includes_.count(fname) || !!inProgress_.count(fname) || (currentFileName.compare(fileName_.c_str()) == 0)) {
    LeaveCriticalSection(&lock_);
    free(fname);
    return true;
  }
  bool rval = false;

  JSContext* cx = GetContext();

  JS_BeginRequest(cx);

  JSScript* _script = JS_CompileFile(cx, GetGlobalObject(), fname);
  if (_script) {
    jsval dummy;
    inProgress_[fname] = true;
    rval = !!JS_ExecuteScript(cx, GetGlobalObject(), _script, &dummy);
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
  // nothing can be registered under an empty name
  if (strlen(evtName) < 1) return false;

  // if there are no events registered under that name at all, then obviously there
  // can't be a specific one registered under that name
  if (functions_.count(evtName) < 1) return false;

  for (FunctionList::iterator it = functions_[evtName].begin(); it != functions_[evtName].end(); it++)
    if (*(*it)->value() == evtFunc) return true;

  return false;
}

void Script::UnregisterEvent(const char* evtName, jsval evtFunc) {
  if (strlen(evtName) < 1) return;

  EnterCriticalSection(&lock_);
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
  if (functions_.count(evtName) > 0 && functions_[evtName].size() == 0) functions_.erase(evtName);

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

void Script::ClearAllEvents(void) {
  EnterCriticalSection(&lock_);
  for (FunctionMap::iterator it = functions_.begin(); it != functions_.end(); it++) ClearEvent(it->first.c_str());
  functions_.clear();
  LeaveCriticalSection(&lock_);
}

void Script::FireEvent(Event* evt) {
  EnterCriticalSection(&Vars.cEventSection);
  evt->owner->events().push_front(evt);
  LeaveCriticalSection(&Vars.cEventSection);

  if (evt->owner && evt->owner->IsRunning()) {
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
  JSContext* cx = rcs->script->GetContext();
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
    SetThreadName(0xFFFFFFFF, script->GetShortFilename());
#endif
    script->Run();
    if (Vars.bDisableCache) {
      sScriptEngine->DisposeScript(script);
    }
  }
  return 0;
}
