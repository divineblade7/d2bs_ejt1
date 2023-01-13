#pragma once

// TODO: Rewrite this mess of crap too

#include "d2bs/script/js32.h"

#include <cstdlib>
#include <cstring>

class DirData {
 public:
  wchar_t name[_MAX_FNAME];
  DirData(const wchar_t* newname) {
    wcscpy_s(name, _MAX_FNAME, newname);
  }
};

CLASS_CTOR(dir);
void dir_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(dir_name);

JSAPI_FUNC(dir_getFiles);
JSAPI_FUNC(dir_getFolders);
JSAPI_FUNC(dir_create);
JSAPI_FUNC(dir_delete);
JSAPI_FUNC(my_openDir);

// clang-format off
static JSPropertySpec dir_props[] = {
  JS_PSG("name",            dir_name,           JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec dir_methods[] = {
  JS_FS("create",           dir_create,         1,  FUNCTION_FLAGS),
  JS_FS("remove",           dir_delete,         1,  FUNCTION_FLAGS),
  JS_FS("getFiles",         dir_getFiles,       1,  FUNCTION_FLAGS),
  JS_FS("getFolders",       dir_getFolders,     1,  FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass folder_class{
    "Folder",                               // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    dir_finalize,           // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    dir_ctor,               // construct
                    nullptr)                // trace
};
