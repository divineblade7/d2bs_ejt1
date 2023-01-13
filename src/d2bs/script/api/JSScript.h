#pragma once

#include "d2bs/script/Script.h"
#include "d2bs/script/js32.h"

#include <windows.h>

CLASS_CTOR(script);

JSAPI_PROP(script_name);
JSAPI_PROP(script_type);
JSAPI_PROP(script_running);
JSAPI_PROP(script_threadid);
JSAPI_PROP(script_memory);

JSAPI_FUNC(script_getNext);
JSAPI_FUNC(script_stop);
JSAPI_FUNC(script_send);
JSAPI_FUNC(script_pause);
JSAPI_FUNC(script_resume);
JSAPI_FUNC(script_join);
JSAPI_FUNC(my_getScript);
JSAPI_FUNC(my_getScripts);

// clang-format off
static JSPropertySpec script_props[] = {
  JS_PSG("name",        script_name,        JSPROP_PERMANENT_VAR),
  JS_PSG("type",        script_type,        JSPROP_PERMANENT_VAR),
  JS_PSG("running",     script_running,     JSPROP_PERMANENT_VAR),
  JS_PSG("threadid",    script_threadid,    JSPROP_PERMANENT_VAR),
  JS_PSG("memory",      script_memory,      JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec script_methods[] = {
  JS_FS("getNext",      script_getNext,     0, FUNCTION_FLAGS),
  JS_FS("pause",        script_pause,       0, FUNCTION_FLAGS),
  JS_FS("resume",       script_resume,      0, FUNCTION_FLAGS),
  JS_FS("stop",         script_stop,        0, FUNCTION_FLAGS),
  JS_FS("join",         script_join,        0, FUNCTION_FLAGS),
  JS_FS("send",         script_send,        1, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass script_class{
    "D2BSScript",                           // name
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
                    script_ctor,            // construct
                    nullptr)                // trace
};
