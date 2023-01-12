#ifndef __FILE_H__
#define __FILE_H__

#include "d2bs/script/js32.h"

char* readLine(FILE* fptr, bool locking);
bool writeValue(FILE* fptr, JSContext* cx, JS::Value value, bool isBinary, bool locking);
FILE* fileOpenRelScript(const wchar_t* filename, const wchar_t* mode, JSContext* cx);
std::wstring getPathRelScript(const wchar_t* filename);
bool isValidPath(const wchar_t* name);
bool is_bom(const char* str);

#endif
