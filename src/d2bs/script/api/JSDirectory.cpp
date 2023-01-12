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

// TODO: Rewrite this crap :(

#define _USE_32BIT_TIME_T

#include "d2bs/script/api/JSDirectory.h"

#include "d2bs/core/File.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/new_util/localization.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

#include <cerrno>
#include <direct.h>
#include <io.h>
// #include "js32.h"

////////////////////////////////////////////////////////////////////////////////
// Directory stuff
////////////////////////////////////////////////////////////////////////////////

EMPTY_CTOR(dir)

JSAPI_FUNC(my_openDir) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1 || !args[0].isString()) {
    args.rval().setNull();
    return JS_TRUE;
  }

  const wchar_t* name = JS_GetStringCharsZ(cx, args[0].toString());

  if (!isValidPath(name)) {
    Log(L"The following path was deemed invalid: %s. (%s, %s)", name, L"JSDirectory.cpp", L"my_openDir");
    return JS_FALSE;
  }

  auto path = (Vars.settings.script_dir / name).make_preferred();
  if (!std::filesystem::exists(path) && !std::filesystem::create_directories(path)) {
    auto n = d2bs::util::wide_to_utf8(name);
    auto p = path.string();
    JS_ReportError(cx, "Couldn't get directory %s, path '%s' not found", n.c_str(), p.c_str());
    return JS_FALSE;
  }

  DirData* d = new DirData(name);
  JSObject* jsdir = BuildObject(cx, &folder_class, dir_methods, dir_props, d);
  args.rval().setObjectOrNull(jsdir);
  return JS_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// dir_getFiles
// searches a directory for files with a specified extension(*.* assumed)
//
// Added by lord2800
////////////////////////////////////////////////////////////////////////////////

JSAPI_FUNC(dir_getFiles) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() > 1) {
    return JS_FALSE;
  }

  std::wstring search = L"*.*";
  if (args.length() == 1 && args[0].isString()) {
    search = JS_GetStringCharsZ(cx, args[0].toString());
  }

  auto self = args.thisv();
  DirData* d = (DirData*)JS_GetPrivate(self.toObjectOrNull());
  if (search.empty()) {
    THROW_ERROR(cx, "Failed to get search string");
  }

  long hFile;
  wchar_t oldpath[_MAX_PATH];
  auto path = (Vars.settings.script_dir / d->name).make_preferred().wstring();

  if (!_wgetcwd(oldpath, _MAX_PATH)) {
    Log(L"Error getting current working directory. (%s, %s)", L"JSDirectory.cpp", L"dir_getFiles");
    return JS_FALSE;
  }
  if (_wchdir(path.c_str()) == -1) {
    Log(L"Changing directory to %s. (%s, %s)", path.c_str(), L"JSDirectory.cpp", L"dir_getFiles");
    return JS_FALSE;
  }

  _wfinddata_t found;
  JSObject* jsarray = JS_NewArrayObject(cx, 0, NULL);
  args.rval().setObjectOrNull(jsarray);

  if ((hFile = _wfindfirst(search.c_str(), &found)) != -1L) {
    JSAutoRequest r(cx);

    int32_t element = 0;
    do {
      if ((found.attrib & _A_SUBDIR)) continue;
      JS::Value file = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, found.name));
      JS_SetElement(cx, jsarray, element++, &file);
    } while (_wfindnext(hFile, &found) == 0);
  }

  if (_wchdir(oldpath) == -1) {
    Log(L"Error changing directory back to %s. (%s, %s)", oldpath, L"JSDirectory.cpp", L"dir_getFiles");
    return JS_FALSE;
  }

  return JS_TRUE;
}

JSAPI_FUNC(dir_getFolders) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() > 1) {
    return JS_FALSE;
  }

  std::wstring search = L"*.*";
  if (args.length() == 1 && args[0].isString()) {
    search = JS_GetStringCharsZ(cx, args[0].toString());
  }

  auto self = args.thisv();
  DirData* d = (DirData*)JS_GetPrivate(self.toObjectOrNull());
  if (search.empty()) THROW_ERROR(cx, "Failed to get search string");

  long hFile;
  wchar_t oldpath[_MAX_PATH];
  auto path = (Vars.settings.script_dir / d->name).make_preferred().wstring();

  if (!_wgetcwd(oldpath, _MAX_PATH)) {
    Log(L"Error getting current working directory. (%s, %s)", L"JSDirectory.cpp", L"dir_getFolders");
    return JS_FALSE;
  }
  if (_wchdir(path.c_str()) == -1) {
    Log(L"Changing directory to %s. (%s, %s)", path.c_str(), L"JSDirectory.cpp", L"dir_getFolders");
    return JS_FALSE;
  }

  _wfinddata_t found;
  JSObject* jsarray = JS_NewArrayObject(cx, 0, NULL);
  args.rval().setObjectOrNull(jsarray);

  if ((hFile = _wfindfirst(search.c_str(), &found)) != -1L) {
    JSAutoRequest r(cx);
    int32_t element = 0;
    do {
      if (!wcscmp(found.name, L"..") || !wcscmp(found.name, L".") || !(found.attrib & _A_SUBDIR)) continue;
      JS::Value file = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, found.name));
      JS_SetElement(cx, jsarray, element++, &file);
    } while (_wfindnext(hFile, &found) == 0);
  }

  if (_wchdir(oldpath) == -1) {
    Log(L"Error changing directory back to %s. (%s, %s)", oldpath, L"JSDirectory.cpp", L"dir_getFolders");
    return JS_FALSE;
  }

  return JS_TRUE;
}

JSAPI_FUNC(dir_create) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1 || !args[0].isString()) {
    args.rval().setNull();
    return JS_TRUE;
  }

  const wchar_t* name = JS_GetStringCharsZ(cx, args[0].toString());

  if (!isValidPath(name)) {
    return JS_FALSE;
  }

  auto self = args.thisv();
  DirData* d = (DirData*)JS_GetPrivate(self.toObjectOrNull());
  auto path = (Vars.settings.script_dir / d->name / name).make_preferred().wstring();

  if (_wmkdir(path.c_str()) == -1 && (errno == ENOENT)) {
    JS_ReportError(cx, "Couldn't create directory %s, path %s not found", name, path.c_str());
    return JS_FALSE;
  } else {
    d = new DirData(name);
    JSObject* jsdir = BuildObject(cx, &folder_class, dir_methods, dir_props, d);
    args.rval().setObjectOrNull(jsdir);
  }
  return JS_TRUE;
}

JSAPI_FUNC(dir_delete) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  DirData* d = (DirData*)JS_GetPrivate(self.toObjectOrNull());

  auto path = (Vars.settings.script_dir / d->name).make_preferred().wstring();

  if (_wrmdir(path.c_str()) == -1) {
    // TODO: Make an optional param that specifies recursive delete
    if (errno == ENOTEMPTY)
      THROW_ERROR(cx, "Tried to delete directory, but it is not empty or is the current working directory");
    if (errno == ENOENT) THROW_ERROR(cx, "Path not found");
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_PROP(dir_getProperty) {
  DirData* d = (DirData*)JS_GetPrivate(obj);

  if (!d) return JS_FALSE;

  JS::Value ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case DIR_NAME:
      vp.setString(JS_InternUCString(cx, d->name));
      break;
  }
  return JS_TRUE;
}

void dir_finalize(JSFreeOp*, JSObject* obj) {
  DirData* d = (DirData*)JS_GetPrivate(obj);
  delete d;
}
