#include "d2bs/utils/Helpers.h"

#include "d2bs/D2BS.h"
#include "d2bs/core/Control.h"
#include "d2bs/core/Core.h"
#include "d2bs/core/Profile.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/diablo/handlers/D2Handlers.h"
#include "d2bs/script/Script.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/utils/StackWalker.h"

#include <DbgHelp.h>
#include <string.h>
#include <tlhelp32.h>
#include <wctype.h>

wchar_t* AnsiToUnicode(const char* str, UINT codepage) {
  wchar_t* buf = NULL;
  int len = MultiByteToWideChar(codepage, 0, str, -1, buf, 0);
  buf = new wchar_t[len];
  MultiByteToWideChar(codepage, 0, str, -1, buf, len);
  return buf;
}

char* UnicodeToAnsi(const wchar_t* str, UINT codepage) {
  char* buf = NULL;
  int len = WideCharToMultiByte(codepage, 0, str, -1, buf, 0, (codepage ? NULL : "?"), NULL);
  buf = new char[len];
  WideCharToMultiByte(codepage, 0, str, -1, buf, len, (codepage ? NULL : "?"), NULL);
  return buf;
}

void StringToLower(char* p) {
  for (; *p; ++p) *p = static_cast<char>(tolower(static_cast<int>(*p)));
}

void StringToLower(wchar_t* p) {
  for (; *p; ++p) *p = towlower(*p);
}

bool StringToBool(const char* str) {
  switch (tolower(str[0])) {
    case 't':
    case '1':
      return true;
    case 'f':
    case '0':
    default:
      return false;
  }
}

bool StringToBool(const wchar_t* str) {
  switch (tolower(str[0])) {
    case 't':
    case '1':
      return true;
    case 'f':
    case '0':
    default:
      return false;
  }
}

void StringReplace(char* str, const char find, const char replace, size_t buflen) {
  for (size_t i = 0; i < buflen; i++) {
    if (str[i] == find) str[i] = replace;
  }
}

void StringReplace(wchar_t* str, const wchar_t find, const wchar_t replace, size_t buflen) {
  for (size_t i = 0; i < buflen; i++) {
    if (str[i] == find) str[i] = replace;
  }
}

bool SwitchToProfile(const wchar_t* profile) {
  if (Vars.bUseProfileScript != TRUE || !Profile::ProfileExists(profile)) {
    return false;
  }

  wchar_t defaultStarter[_MAX_FNAME] = L"", defaultConsole[_MAX_FNAME] = L"", defaultGame[_MAX_FNAME] = L"",
          scriptPath[_MAX_PATH] = L"";
  auto path = (Vars.working_dir / "d2bs.ini").wstring();
  auto file = path.c_str();

  GetPrivateProfileStringW(profile, L"ScriptPath", L"scripts", scriptPath, _MAX_PATH, file);
  GetPrivateProfileStringW(profile, L"DefaultConsoleScript", L"", defaultConsole, _MAX_FNAME, file);
  GetPrivateProfileStringW(profile, L"DefaultGameScript", L"", defaultGame, _MAX_FNAME, file);
  GetPrivateProfileStringW(profile, L"DefaultStarterScript", L"", defaultStarter, _MAX_FNAME, file);

  wcscpy_s(Vars.szProfile, 256, profile);
  Vars.script_dir = Vars.working_dir / scriptPath;

  if (wcslen(defaultConsole) > 0) {
    wcscpy_s(Vars.szConsole, _MAX_FNAME, defaultConsole);
  }
  if (wcslen(defaultGame) > 0) {
    wcscpy_s(Vars.szDefault, _MAX_FNAME, defaultGame);
  }
  if (wcslen(defaultStarter) > 0) {
    wcscpy_s(Vars.szStarter, _MAX_FNAME, defaultStarter);
  }

  Vars.bUseProfileScript = FALSE;
  // Reload();
  return true;
}

const wchar_t* GetStarterScriptName(void) {
  return (ClientState() == ClientStateInGame ? Vars.szDefault
          : ClientState() == ClientStateMenu ? Vars.szStarter
                                             : NULL);
}

ScriptType GetStarterScriptState(void) {
  // the default return is InGame because that's the least harmful of the options
  return (ClientState() == ClientStateInGame ? ScriptType::InGame
          : ClientState() == ClientStateMenu ? ScriptType::OutOfGame
                                             : ScriptType::InGame);
}

bool ExecCommand(const wchar_t* command) {
  sScriptEngine->RunCommand(command);
  return true;
}

bool StartScript(const wchar_t* scriptname, ScriptType type) {
  auto path = (Vars.script_dir / scriptname).make_preferred().wstring();
  Script* script = sScriptEngine->CompileFile(path.c_str(), type);
  return (script && script->BeginThread(ScriptThread));
}

void Reload(void) {
  if (sScriptEngine->GetCount() > 0) {
    Print(L"\u00FFc2D2BS\u00FFc0 :: Stopping all scripts");
  }
  sScriptEngine->StopAll();

  if (Vars.bDisableCache != TRUE) {
    Print(L"\u00FFc2D2BS\u00FFc0 :: Flushing the script cache");
  }
  sScriptEngine->FlushCache();

  // wait for things to catch up
  Sleep(500);

  if (!Vars.bUseProfileScript) {
    const wchar_t* script = GetStarterScriptName();
    if (StartScript(script, GetStarterScriptState())) {
      Print(L"\u00FFc2D2BS\u00FFc0 :: Started %s", script);
    } else {
      Print(L"\u00FFc2D2BS\u00FFc0 :: Failed to start %s", script);
    }
  }
}

bool ProcessCommand(const wchar_t* command, bool unprocessedIsCommand) {
  bool result = false;
  wchar_t* buf = _wcsdup(command);
  wchar_t* next_token1 = NULL;
  wchar_t* argv = wcstok_s(buf, L" ", &next_token1);

  // no command?
  if (argv == NULL) return false;

  if (_wcsicmp(argv, L"start") == 0) {
    const wchar_t* script = GetStarterScriptName();
    if (StartScript(script, GetStarterScriptState()))
      Print(L"\u00FFc2D2BS\u00FFc0 :: Started %s", script);
    else
      Print(L"\u00FFc2D2BS\u00FFc0 :: Failed to start %s", script);
    result = true;
  } else if (_wcsicmp(argv, L"stop") == 0) {
    if (sScriptEngine->GetCount() > 0) {
      Print(L"\u00FFc2D2BS\u00FFc0 :: Stopping all scripts");
    }
    sScriptEngine->StopAll();
    result = true;
  } else if (_wcsicmp(argv, L"flush") == 0) {
    if (Vars.bDisableCache != TRUE) {
      Print(L"\u00FFc2D2BS\u00FFc0 :: Flushing the script cache");
    }
    sScriptEngine->FlushCache();
    result = true;
  } else if (_wcsicmp(argv, L"load") == 0) {
    const wchar_t* script = command + 5;
    if (StartScript(script, GetStarterScriptState()))
      Print(L"\u00FFc2D2BS\u00FFc0 :: Started %s", script);
    else
      Print(L"\u00FFc2D2BS\u00FFc0 :: Failed to start %s", script);
    result = true;
  } else if (_wcsicmp(argv, L"reload") == 0) {
    Reload();
    result = true;
  }
#if DEBUG
  else if (_wcsicmp(argv, L"crash") == 0) {
    DWORD zero = 0;
    double value = 1 / zero;
    Print(L"%d", value);
  } else if (_wcsicmp(argv, L"profile") == 0) {
    const wchar_t* profile = command + 8;
    if (SwitchToProfile(profile))
      Print(L"ÿc2D2BSÿc0 :: Switched to profile %s", profile);
    else
      Print(L"ÿc2D2BSÿc0 :: Profile %s not found", profile);
    result = true;
  }
#endif
  else if (_wcsicmp(argv, L"exec") == 0 && !unprocessedIsCommand) {
    ExecCommand(command + 5);
    result = true;
  } else if (unprocessedIsCommand) {
    ExecCommand(command);
    result = true;
  }
  free(buf);
  return result;
}

SYMBOL_INFO* GetSymFromAddr(HANDLE hProcess, DWORD64 addr) {
  char* symbols = new char[sizeof(SYMBOL_INFO) + 512];
  memset(symbols, 0, sizeof(SYMBOL_INFO) + 512);

  SYMBOL_INFO* sym = (SYMBOL_INFO*)(symbols);
  sym->SizeOfStruct = sizeof(SYMBOL_INFO);
  sym->MaxNameLen = 512;

  DWORD64 dummy;
  bool success = SymFromAddr(hProcess, addr, &dummy, sym) == TRUE ? true : false;
  if (!success) {
    delete[] symbols;
    sym = NULL;
  }

  return sym;
}

IMAGEHLP_LINE64* GetLineFromAddr(HANDLE hProcess, DWORD64 addr) {
  IMAGEHLP_LINE64* line = new IMAGEHLP_LINE64;
  line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  DWORD dummy;
  bool success = SymGetLineFromAddr64(hProcess, addr, &dummy, line) == TRUE ? true : false;
  if (!success) {
    delete line;
    line = NULL;
  }
  return line;
}

char* DllLoadAddrStrs() {
  const char* dlls[] = {"D2Client.DLL", "D2Common.DLL", "D2Gfx.DLL",    "D2Lang.DLL", "D2Win.DLL",
                        "D2Net.DLL",    "D2Game.DLL",   "D2Launch.DLL", "Fog.DLL",    "BNClient.DLL",
                        "Storm.DLL",    "D2Cmp.DLL",    "D2Multi.DLL"};
  size_t strMaxLen;
  char* result;
  char lineBuf[80];
  unsigned int i;

  strMaxLen = sizeof(lineBuf) * sizeof(dlls) / sizeof(dlls[0]);
  result = (char*)malloc(strMaxLen);

  result[0] = '\0';

  for (i = 0; i < sizeof(dlls) / sizeof(dlls[0]); ++i) {
    sprintf_s(lineBuf, sizeof(lineBuf), "%s loaded at: 0x%p.", dlls[i], GetModuleHandle(dlls[i]));
    strcat_s(result, strMaxLen, lineBuf);
    if (i != (sizeof(dlls) / sizeof(dlls[0]) - 1)) {
      strcat_s(result, strMaxLen, "\n");
    }
  }

  return result;
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ptrs) {
  GetStackWalk();

  EXCEPTION_RECORD* rec = ptrs->ExceptionRecord;
  CONTEXT* ctx = ptrs->ContextRecord;
  DWORD base = Vars.pModule ? Vars.pModule->dwBaseAddress : (DWORD)Vars.hModule;

  int len;
  char* szString;
  char* dllAddrs;

  len = _scprintf(
      "EXCEPTION!\n*** 0x%08x at 0x%p\n"
      "D2BS loaded at: 0x%08x\n"
      "Registers:\n"
      "\tEIP: 0x%08x, ESP: 0x%08x\n"
      "\tCS: 0x%04x, DS: 0x%04x, ES: 0x%04x, SS: 0x%04x, FS: 0x%04x, GS: 0x%04x\n"
      "\tEAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x, ESI: 0x%08x, EDI: 0x%08x, EBP: 0x%08x, FLG: 0x%08x\n",
      rec->ExceptionCode, rec->ExceptionAddress, base, ctx->Eip, ctx->Esp, ctx->SegCs, ctx->SegDs, ctx->SegEs,
      ctx->SegSs, ctx->SegFs, ctx->SegGs, ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi, ctx->Ebp,
      ctx->EFlags);

  szString = new char[len + 1];
  sprintf_s(
      szString, len + 1,
      "EXCEPTION!\n*** 0x%08x at 0x%p\n"
      "D2BS loaded at: 0x%08x\n"
      "Registers:\n"
      "\tEIP: 0x%08x, ESP: 0x%08x\n"
      "\tCS: 0x%04x, DS: 0x%04x, ES: 0x%04x, SS: 0x%04x, FS: 0x%04x, GS: 0x%04x\n"
      "\tEAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x, ESI: 0x%08x, EDI: 0x%08x, EBP: 0x%08x, FLG: 0x%08x\n",
      rec->ExceptionCode, rec->ExceptionAddress, base, ctx->Eip, ctx->Esp, ctx->SegCs, ctx->SegDs, ctx->SegEs,
      ctx->SegSs, ctx->SegFs, ctx->SegGs, ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi, ctx->Ebp,
      ctx->EFlags);

  dllAddrs = DllLoadAddrStrs();
  Log(L"%hs\n%hs", szString, dllAddrs);

  free(dllAddrs);
  delete[] szString;
  return EXCEPTION_EXECUTE_HANDLER;
}

int __cdecl _purecall(void) {
  GetStackWalk();
  return 0;
}

class MyStackWalker : public StackWalker {
 public:
  MyStackWalker() : StackWalker() {}

 protected:
  virtual void OnOutput(LPCSTR szText) {
    Log(L"%hs", szText);
  }
};

std::vector<DWORD> GetThreadIds() {
  std::vector<DWORD> threadIds;

  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());

  if (hSnapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &threadEntry)) {
      do {
        if (threadEntry.th32OwnerProcessID == GetCurrentProcessId()) threadIds.push_back(threadEntry.th32ThreadID);

      } while (Thread32Next(hSnapshot, &threadEntry));
    }
  }
  CloseHandle(hSnapshot);

  return threadIds;
}

void ResumeProcess() {
  std::vector<DWORD> threadIds = GetThreadIds();

  for (std::vector<DWORD>::iterator it = threadIds.begin(); it != threadIds.end(); ++it) {
    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, *it);
    ResumeThread(hThread);
    CloseHandle(hThread);
  }
}

bool GetStackWalk() {
  std::vector<DWORD> threadIds = GetThreadIds();
  DWORD current = GetCurrentThreadId();

  MyStackWalker sw;
  for (std::vector<DWORD>::iterator it = threadIds.begin(); it != threadIds.end(); ++it) {
    if (*it == current) continue;

    HANDLE hThread = OpenThread(THREAD_GET_CONTEXT, false, *it);

    if (hThread == INVALID_HANDLE_VALUE) return false;

    Log(L"Stack Walk Thread: %d", *it);
    sw.ShowCallstack(hThread);
  }

  Log(L"Stack Walk Thread: %d", current);
  sw.ShowCallstack();

  return true;
}
