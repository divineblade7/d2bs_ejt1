#pragma once

#include "d2bs/script/js32.h"

#include <windows.h>

struct myPresetUnit {
  DWORD dwType;
  DWORD dwRoomX;
  DWORD dwRoomY;
  DWORD dwPosX;
  DWORD dwPosY;
  DWORD dwId;
  DWORD dwLevel;
};

CLASS_CTOR(presetunit);
void presetunit_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(presetunit_id);
JSAPI_PROP(presetunit_type);
JSAPI_PROP(presetunit_level);
JSAPI_PROP(presetunit_x);
JSAPI_PROP(presetunit_y);
JSAPI_PROP(presetunit_roomx);
JSAPI_PROP(presetunit_roomy);

JSAPI_FUNC(my_getPresetUnit);
JSAPI_FUNC(my_getPresetUnits);

// clang-format off
static JSPropertySpec presetunit_props[] = {
  JS_PSG("id",          presetunit_id,          JSPROP_PERMANENT_VAR),
  JS_PSG("type",        presetunit_type,        JSPROP_PERMANENT_VAR),
  JS_PSG("level",       presetunit_level,       JSPROP_PERMANENT_VAR),
  JS_PSG("x",           presetunit_x,           JSPROP_PERMANENT_VAR),
  JS_PSG("y",           presetunit_y,           JSPROP_PERMANENT_VAR),
  JS_PSG("roomx",       presetunit_roomx,       JSPROP_PERMANENT_VAR),
  JS_PSG("roomy",       presetunit_roomy,       JSPROP_PERMANENT_VAR),
  JS_PS_END
};
// clang-format on

static JSClass presetunit_class{
    "PresetUnit",                           // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    presetunit_finalize,    // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    presetunit_ctor,        // construct
                    nullptr)                // trace
};
