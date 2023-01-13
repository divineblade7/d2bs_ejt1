#pragma once

#include "d2bs/script/js32.h"

CLASS_CTOR(filetools);

JSAPI_FUNC(filetools_remove);
JSAPI_FUNC(filetools_rename);
JSAPI_FUNC(filetools_copy);
JSAPI_FUNC(filetools_exists);
JSAPI_FUNC(filetools_readText);
JSAPI_FUNC(filetools_writeText);
JSAPI_FUNC(filetools_appendText);

// clang-format off
static JSFunctionSpec filetools_s_methods[] = {
  JS_FS("remove",           filetools_remove,         1,    FUNCTION_FLAGS),
  JS_FS("rename",           filetools_rename,         2,    FUNCTION_FLAGS),
  JS_FS("copy",             filetools_copy,           2,    FUNCTION_FLAGS),
  JS_FS("exists",           filetools_exists,         1,    FUNCTION_FLAGS),
  JS_FS("readText",         filetools_readText,       1,    FUNCTION_FLAGS),
  JS_FS("writeText",        filetools_writeText,      2,    FUNCTION_FLAGS),
  JS_FS("appendText",       filetools_appendText,     2,    FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass filetools_class{
    "FileTools",                            // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    nullptr,                // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    filetools_ctor,         // construct
                    nullptr)                // trace
};
