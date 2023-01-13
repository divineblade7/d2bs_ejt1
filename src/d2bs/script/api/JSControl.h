#pragma once

#include "d2bs/core/Control.h"
#include "d2bs/diablo/D2Ptrs.h"
#include "d2bs/script/js32.h"

#include <windows.h>

struct ControlData {
  DWORD _dwPrivate;
  Control* pControl;

  DWORD dwType;
  DWORD dwX;
  DWORD dwY;
  DWORD dwSizeX;
  DWORD dwSizeY;
};

CLASS_CTOR(control);
void control_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(control_type);
JSAPI_PROP(control_text);
JSAPI_STRICT_PROP(control_text_setter);
JSAPI_PROP(control_state);
JSAPI_STRICT_PROP(control_state_setter);
JSAPI_PROP(control_disabled);
JSAPI_STRICT_PROP(control_disabled_setter);
JSAPI_PROP(control_password);
JSAPI_PROP(control_x);
JSAPI_PROP(control_y);
JSAPI_PROP(control_xsize);
JSAPI_PROP(control_ysize);
JSAPI_PROP(control_cursorpos);
JSAPI_STRICT_PROP(control_cursorpos_setter);
JSAPI_PROP(control_selectstart);
JSAPI_PROP(control_selectend);

JSAPI_FUNC(control_getNext);
JSAPI_FUNC(control_click);
JSAPI_FUNC(control_setText);
JSAPI_FUNC(control_getText);

// clang-format off
static JSPropertySpec control_props[] = {
    JS_PSG("type",        control_type,                                 JSPROP_PERMANENT_VAR),
    JS_PSGS("text",       control_text, control_text_setter,            JSPROP_PERMANENT_VAR),
    JS_PSGS("state",      control_state, control_state_setter,          JSPROP_PERMANENT_VAR),
    JS_PSGS("disabled",   control_disabled, control_disabled_setter,    JSPROP_PERMANENT_VAR),
    JS_PSG("password",    control_password,                             JSPROP_PERMANENT_VAR),
    JS_PSG("x",           control_x,                                    JSPROP_PERMANENT_VAR),
    JS_PSG("y",           control_y,                                    JSPROP_PERMANENT_VAR),
    JS_PSG("xsize",       control_xsize,                                JSPROP_PERMANENT_VAR),
    JS_PSG("ysize",       control_ysize,                                JSPROP_PERMANENT_VAR),
    JS_PSGS("cursorpos",  control_cursorpos, control_cursorpos_setter,  JSPROP_PERMANENT_VAR),
    JS_PSG("selectstart", control_selectstart,                          JSPROP_PERMANENT_VAR),
    JS_PSG("selectend",   control_selectend,                            JSPROP_PERMANENT_VAR),
    JS_PS_END
};

static JSFunctionSpec control_funcs[] = {
  JS_FS("getNext",        control_getNext,      0,  FUNCTION_FLAGS),
  JS_FS("click",          control_click,        0,  FUNCTION_FLAGS),
  JS_FS("setText",        control_setText,      1,  FUNCTION_FLAGS),
  JS_FS("getText",        control_getText,      0,  FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass control_class{
    "Control",                              // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    control_finalize,       // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    control_ctor,           // construct
                    nullptr)                // trace
};
