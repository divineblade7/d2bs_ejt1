#pragma once

#include "d2bs/script/Script.h"

bool GetStackWalk();
void StringReplace(char* str, const char find, const char replace, size_t buflen);
void StringReplace(wchar_t* str, const wchar_t find, const wchar_t replace, size_t buflen);

bool ExecCommand(const wchar_t* command);
bool StartScript(const wchar_t* script, ScriptType type);
void Reload(void);
bool ProcessCommand(const wchar_t* command, bool unprocessedIsCommand);
void ResumeProcess();

const wchar_t* GetStarterScriptName(void);
ScriptType GetStarterScriptState(void);

#ifdef DEBUG
LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ptrs);
int __cdecl _purecall(void);
#endif
