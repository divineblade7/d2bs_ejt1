/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "d2bs/core/File.h"

#include "d2bs/engine.h"
#include "d2bs/utils/Helpers.h"

#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

char* readLine(FILE* fptr, bool locking) {
  if (feof(fptr)) return NULL;
  std::string buffer;
  char c = 0;
  // grab all the characters in this line
  do {
    if (locking)
      c = (char)fgetc(fptr);
    else
      c = (char)_fgetc_nolock(fptr);
    // append the new character unless it's a carriage return
    if (c != '\r' && c != '\n' && !feof(fptr)) buffer.append(1, c);
  } while (!feof(fptr) && c != '\n');
  return _strdup(buffer.c_str());
}

bool writeValue(FILE* fptr, JSContext* cx, jsval value, bool isBinary, bool locking) {
  char* str;
  int len = 0, result;
  int32 ival = 0;
  jsdouble dval = 0;
  bool bval;

  switch (JS_TypeOfValue(cx, value)) {
    case JSTYPE_VOID:
    case JSTYPE_NULL:
      if (locking)
        result = fwrite(&ival, sizeof(int), 1, fptr);
      else
        result = _fwrite_nolock(&ival, sizeof(int), 1, fptr);
      if (result == 1) return true;
      break;
    case JSTYPE_STRING:
      str = JS_EncodeStringToUTF8(cx, JSVAL_TO_STRING(value));
      if (locking)
        result = fwrite(str, sizeof(char), strlen(str), fptr);
      else
        result = _fwrite_nolock(str, sizeof(char), strlen(str), fptr);

      JS_free(cx, str);
      return (int)strlen(str) == result;
      break;
    case JSTYPE_NUMBER:
      if (isBinary) {
        if (JSVAL_IS_DOUBLE(value)) {
          if (JS_ValueToNumber(cx, value, &dval)) {
            if (locking)
              result = fwrite(&dval, sizeof(jsdouble), 1, fptr);
            else
              result = _fwrite_nolock(&dval, sizeof(jsdouble), 1, fptr);
            return result == 1;
          } else
            return false;
        } else if (JSVAL_IS_INT(value)) {
          if (JS_ValueToInt32(cx, value, &ival)) {
            if (locking)
              result = fwrite(&ival, sizeof(int32), 1, fptr);
            else
              result = _fwrite_nolock(&dval, sizeof(int32), 1, fptr);
            return result == 1;
          } else
            return false;
        }
      } else {
        if (JSVAL_IS_DOUBLE(value)) {
          if (JS_ValueToNumber(cx, value, &dval) == JS_FALSE) return false;
          // jsdouble will never be a 64-char string, but I'd rather be safe than sorry
          str = new char[64];
          sprintf_s(str, 64, "%.16f", dval);
          len = strlen(str);
          if (locking)
            result = fwrite(str, sizeof(char), len, fptr);
          else
            result = _fwrite_nolock(str, sizeof(char), len, fptr);
          delete[] str;
          if (result == len) return true;
        } else if (JSVAL_IS_INT(value)) {
          if (JS_ValueToInt32(cx, value, &ival) == JS_FALSE) return false;
          str = new char[16];
          _itoa_s(ival, str, 16, 10);
          len = strlen(str);
          if (locking)
            result = fwrite(str, sizeof(char), len, fptr);
          else
            result = _fwrite_nolock(str, sizeof(char), len, fptr);
          delete[] str;
          if (result == len) return true;
        }
      }
      break;
    case JSTYPE_BOOLEAN:
      if (!isBinary) {
        bval = !!JSVAL_TO_BOOLEAN(value);
        const char* boolstr = bval ? "true" : "false";
        if (locking)
          result = fwrite(boolstr, sizeof(char), strlen(boolstr), fptr);
        else
          result = _fwrite_nolock(boolstr, sizeof(char), strlen(boolstr), fptr);
        return (int)strlen(boolstr) == result;
      } else {
        bval = !!JSVAL_TO_BOOLEAN(value);
        if (locking)
          result = fwrite(&bval, sizeof(bool), 1, fptr);
        else
          result = _fwrite_nolock(&bval, sizeof(bool), 1, fptr);
        return result == 1;
      }
      break;
      /*		case JSTYPE_OBJECT:
                              JSObject *arr = JSVAL_TO_OBJECT(value);
                              if(JS_IsArrayObject(cx, arr)) {
                                      JS_GetArrayLength(cx, arr, &uival);
                                      for(jsuint i = 0; i < uival; i++)
                                      {
                                              jsval val;
                                              JS_GetElement(cx, arr, i, &val);
                                              if(!writeValue(fptr, cx, val, isBinary))
                                                      return false;
                                      }
                                      return true;
                              }
                              else
                              {
                                      JSString* jsstr = JS_ValueToString(cx, value);
                                      str = JS_EncodeString(cx,jsstr);
                                      if(locking)
                                              result = fwrite(str, sizeof(char), strlen(str), fptr);
                                      else
                                              result = _fwrite_nolock(str, sizeof(char), strlen(str), fptr);
                                      return strlen(str) == result;
                              }
                              break;
      */
  }
  return false;
}

FILE* fileOpenRelScript(const wchar_t* filename, const wchar_t* mode, JSContext* cx) {
  FILE* f;
  auto path = getPathRelScript(filename);

  // Open the file
  if (_wfopen_s(&f, path.c_str(), mode) != 0 || f == NULL) {
    char message[128];
    _strerror_s(message, 128, NULL);
    JS_ReportError(cx, "Couldn't open file %ls: %s", filename, message);
    return NULL;
  }

  return f;
}

/** Get the full path relative to the script path. Does the validation check.
 *
 * \param filename Name of the file to open relative to the script folder.
 *
 * \param bufLen Length of the output buffer.
 *
 * \param fullPath Buffer where the full path will be stored. Empty string if
 *    location is invalid.
 *
 * \return fullPath on success or NULL on failure.
 */
std::wstring getPathRelScript(const wchar_t* filename) {
  return (Vars.script_dir / filename).make_preferred().wstring();
}

/** Check that the full path of the script path is the prefix of the fullpath
 * of name. Also checks that there is no ..\ or ../ sequences or ":?*<>| chars.
 *
 * \param name The file/path to validate.
 *
 * \return true if path is valid, false otherwise.
 */
bool isValidPath(const wchar_t* name) {
  if (getPathRelScript(name).empty()) {
    return false;
  }

  return (!wcsstr(name, L"..\\") && !wcsstr(name, L"../") && (wcscspn(name, L"\":?*<>|") == wcslen(name)));
}

bool is_bom(const char* str) {
  return str[0] == static_cast<char>(0xEF) && str[1] == static_cast<char>(0xBB) && str[2] == static_cast<char>(0xBF);
}
