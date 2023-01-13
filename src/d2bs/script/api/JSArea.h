#pragma once

#include "d2bs/script/js32.h"

#include <windows.h>

class ApiArea {
 public:
  ApiArea() = default;
  ~ApiArea() = default;

  DWORD AreaId;
  DWORD Exits;
  JSObject* ExitArray;
};

CLASS_CTOR(area);
void area_finalize(JSFreeOp* fop, JSObject* obj);

// This should really have been the constructor lol, can't change that now...
JSAPI_FUNC(my_getArea);

JSAPI_PROP(area_id);
JSAPI_PROP(area_name);
JSAPI_PROP(area_x);
JSAPI_PROP(area_y);
JSAPI_PROP(area_xsize);
JSAPI_PROP(area_ysize);
JSAPI_PROP(area_exits);

// clang-format off
static JSPropertySpec area_props[] = {
    JS_PSG("id",      area_id,      JSPROP_PERMANENT_VAR),
    JS_PSG("name",    area_name,    JSPROP_PERMANENT_VAR),
    JS_PSG("x",       area_x,       JSPROP_PERMANENT_VAR),
    JS_PSG("y",       area_y,       JSPROP_PERMANENT_VAR),
    JS_PSG("xsize",   area_xsize,   JSPROP_PERMANENT_VAR),
    JS_PSG("ysize",   area_ysize,   JSPROP_PERMANENT_VAR),
    JS_PSG("exits",   area_exits,   JSPROP_PERMANENT_VAR),
    JS_PS_END
};
//clang-format on

static JSClass area_class{
    "Area",                                 // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    area_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    area_ctor,              // construct
                    nullptr)                // trace
};
