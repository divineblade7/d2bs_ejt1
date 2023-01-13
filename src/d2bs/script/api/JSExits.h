#pragma once

#include "d2bs/script/js32.h"

#include <windows.h>

struct myExit {
  DWORD x;
  DWORD y;
  DWORD id;
  DWORD type;
  DWORD tileid;
  DWORD level;
};

CLASS_CTOR(exit);
void exit_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(exit_type);
JSAPI_PROP(exit_tileid);
JSAPI_PROP(exit_level);
JSAPI_PROP(exit_x);
JSAPI_PROP(exit_y);
JSAPI_PROP(exit_target);

// clang-format off
static JSPropertySpec exit_props[] = {
  JS_PSG("type",          exit_type,      JSPROP_PERMANENT_VAR),
  JS_PSG("tileid",        exit_tileid,    JSPROP_PERMANENT_VAR),
  JS_PSG("level",         exit_level,     JSPROP_PERMANENT_VAR),
  JS_PSG("x",             exit_x,         JSPROP_PERMANENT_VAR),
  JS_PSG("y",             exit_y,         JSPROP_PERMANENT_VAR),
  JS_PSG("target",        exit_target,    JSPROP_PERMANENT_VAR),
  JS_PS_END
};
// clang-format on

static JSClass exit_class{
    "Exit",                                 // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    exit_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    exit_ctor,              // construct
                    nullptr)                // trace
};
