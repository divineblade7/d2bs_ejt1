#pragma once

// TODO: Rewrite this mess of crap too

#include "d2bs/script/js32.h"

#include <cstdlib>
#include <cstring>

CLASS_CTOR(dir);

JSAPI_FUNC(dir_getFiles);
JSAPI_FUNC(dir_getFolders);
JSAPI_FUNC(dir_create);
JSAPI_FUNC(dir_delete);
JSAPI_FUNC(my_openDir);

JSAPI_PROP(dir_getProperty);

void dir_finalize(JSFreeOp* fop, JSObject* obj);

class DirData {
 public:
  wchar_t name[_MAX_FNAME];
  DirData(const wchar_t* newname) {
    wcscpy_s(name, _MAX_FNAME, newname);
  }
};

//////////////////////////////////////////////////////////////////
// directory stuff
//////////////////////////////////////////////////////////////////

enum { DIR_NAME };

static JSPropertySpec dir_props[] = {
    {"name", DIR_NAME, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(dir_getProperty), JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}};

static JSFunctionSpec dir_methods[] = {JS_FS("create", dir_create, 1, FUNCTION_FLAGS),
                                       JS_FS("remove", dir_delete, 1, FUNCTION_FLAGS),
                                       JS_FS("getFiles", dir_getFiles, 1, FUNCTION_FLAGS),
                                       JS_FS("getFolders", dir_getFolders, 1, FUNCTION_FLAGS), JS_FS_END};

static JSClass folder_class = {"Folder", JSCLASS_HAS_PRIVATE,
                               JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, dir_finalize, dir_ctor)};
