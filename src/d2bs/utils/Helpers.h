#pragma once

#include "d2bs/script/Script.h"

bool GetStackWalk();
char* UnicodeToAnsi(const wchar_t* str, UINT codepage = CP_UTF8);
void StringToLower(char* p);
void StringToLower(wchar_t* p);
bool StringToBool(const char* str);
bool StringToBool(const wchar_t* str);
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
