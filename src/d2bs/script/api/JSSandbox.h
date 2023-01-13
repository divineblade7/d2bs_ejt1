#pragma once

#include "d2bs/script/Script.h"

struct sandbox {
  JSRuntime* runtime;
  JSContext* context;
  JSObject* innerObj;
  IncludeList list;
};

JSAPI_FUNC(sandbox_ctor);
void sandbox_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(sandbox_addProperty);
JSAPI_PROP(sandbox_delProperty);
JSAPI_PROP(sandbox_getProperty);
JSAPI_STRICT_PROP(sandbox_setProperty);

JSAPI_FUNC(sandbox_eval);
JSAPI_FUNC(sandbox_include);
JSAPI_FUNC(sandbox_isIncluded);
JSAPI_FUNC(sandbox_clear);

// clang-format off
static JSFunctionSpec sandbox_methods[] = {
  JS_FS("evaluate",       sandbox_eval,           1, FUNCTION_FLAGS),
  JS_FS("include",        sandbox_include,        1, FUNCTION_FLAGS),
  JS_FS("isIncluded",     sandbox_isIncluded,     1, FUNCTION_FLAGS),
  JS_FS("clearScope",     sandbox_clear,          0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass sandbox_class{
    "Sandbox",                            // name
    JSCLASS_HAS_PRIVATE,                  // flags
    JSCLASS_METHODS(sandbox_addProperty,  // addProperty
                    sandbox_delProperty,  // delProperty
                    sandbox_getProperty,  // getProperty
                    sandbox_setProperty,  // setProperty
                    JS_EnumerateStub,     // enumerate
                    JS_ResolveStub,       // resolve
                    JS_ConvertStub,       // mayResolve
                    sandbox_finalize,     // finalize
                    nullptr,              // call
                    nullptr,              // hasInstance
                    sandbox_ctor,         // construct
                    nullptr)              // trace
};
