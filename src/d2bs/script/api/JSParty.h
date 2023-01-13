#pragma once

#include "d2bs/script/js32.h"

CLASS_CTOR(party);

JSAPI_PROP(party_gid);
JSAPI_PROP(party_name);
JSAPI_PROP(party_classid);
JSAPI_PROP(party_area);
JSAPI_PROP(party_level);
JSAPI_PROP(party_x);
JSAPI_PROP(party_y);
JSAPI_PROP(party_life);
JSAPI_PROP(party_partyflag);
JSAPI_PROP(party_partyid);

JSAPI_FUNC(party_getNext);
JSAPI_FUNC(my_getParty);

// clang-format off
static JSPropertySpec party_props[] = {
  JS_PSG("gid",           party_gid,            JSPROP_PERMANENT_VAR),
  JS_PSG("name",          party_name,           JSPROP_PERMANENT_VAR),
  JS_PSG("classid",       party_classid,        JSPROP_PERMANENT_VAR),
  JS_PSG("area",          party_area,           JSPROP_PERMANENT_VAR),
  JS_PSG("level",         party_level,          JSPROP_PERMANENT_VAR),
  JS_PSG("x",             party_x,              JSPROP_PERMANENT_VAR),
  JS_PSG("y",             party_y,              JSPROP_PERMANENT_VAR),
  JS_PSG("life",          party_life,           JSPROP_PERMANENT_VAR),
  JS_PSG("partyflag",     party_partyflag,      JSPROP_PERMANENT_VAR),
  JS_PSG("partyid",       party_partyid,        JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec party_methods[] = {
  JS_FS("getNext",        party_getNext,        0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass party_class{
    "Party",                                // name
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
                    party_ctor,             // construct
                    nullptr)                // trace
};
