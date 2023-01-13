#pragma once

#include "d2bs/script/js32.h"

//////////////////////////////////////////////////////////////////
// socket stuff
//////////////////////////////////////////////////////////////////

CLASS_CTOR(socket);
void socket_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(socket_readable);
JSAPI_PROP(socket_writable);

JSAPI_FUNC(socket_open);
JSAPI_FUNC(socket_close);
JSAPI_FUNC(socket_send);
JSAPI_FUNC(socket_read);

// clang-format off
static JSPropertySpec socket_props[] = {
  JS_PSG("readable",    socket_readable,    JSPROP_PERMANENT_VAR),
  JS_PSG("writeable",   socket_writable,    JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec socket_methods[] = {
  JS_FN("close",        socket_close,       0, FUNCTION_FLAGS),
  JS_FN("send",         socket_send,        1, FUNCTION_FLAGS),
  JS_FN("read",         socket_read,        0, FUNCTION_FLAGS),
  JS_FS_END
};

static JSFunctionSpec socket_s_methods[] = {
  JS_FN("open",         socket_open,        2, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass socket_class{
    "Socket",                               // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    socket_finalize,        // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    socket_ctor,            // construct
                    nullptr)                // trace
};
