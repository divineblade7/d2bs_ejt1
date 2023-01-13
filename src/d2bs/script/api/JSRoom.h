#pragma once

#include "d2bs/core/Room.h"
#include "d2bs/script/js32.h"

CLASS_CTOR(room);

JSAPI_PROP(room_area);
JSAPI_PROP(room_level);
JSAPI_PROP(room_number);
JSAPI_PROP(room_subnumber);
JSAPI_PROP(room_x);
JSAPI_PROP(room_y);
JSAPI_PROP(room_xsize);
JSAPI_PROP(room_ysize);
JSAPI_PROP(room_correcttomb);

JSAPI_FUNC(room_getNext);
JSAPI_FUNC(room_getPresetUnits);
JSAPI_FUNC(room_getCollision);
JSAPI_FUNC(room_getCollisionTypeArray);
JSAPI_FUNC(room_getNearby);
JSAPI_FUNC(room_getStat);
JSAPI_FUNC(room_getFirst);
JSAPI_FUNC(room_unitInRoom);
JSAPI_FUNC(room_reveal);

JSAPI_FUNC(my_getRoom);

// clang-format off
static JSPropertySpec room_props[] = {
  JS_PSG("area",              room_area,          JSPROP_PERMANENT_VAR),
  JS_PSG("level",             room_level,         JSPROP_PERMANENT_VAR),
  JS_PSG("number",            room_number,        JSPROP_PERMANENT_VAR),
  JS_PSG("subnumber",         room_subnumber,     JSPROP_PERMANENT_VAR),
  JS_PSG("x",                 room_x,             JSPROP_PERMANENT_VAR),
  JS_PSG("y",                 room_y,             JSPROP_PERMANENT_VAR),
  JS_PSG("xsize",             room_xsize,         JSPROP_PERMANENT_VAR),
  JS_PSG("ysize",             room_ysize,         JSPROP_PERMANENT_VAR),
  JS_PSG("correcttomb",       room_correcttomb,   JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec room_methods[] = {
  JS_FS("getNext",            room_getNext,                 0, FUNCTION_FLAGS),
  JS_FS("reveal",             room_reveal,                  1, FUNCTION_FLAGS),
  JS_FS("getPresetUnits",     room_getPresetUnits,          0, FUNCTION_FLAGS),
  JS_FS("getCollision",       room_getCollision,            0, FUNCTION_FLAGS),
  JS_FS("getCollisionA",      room_getCollisionTypeArray,   0, FUNCTION_FLAGS),
  JS_FS("getNearby",          room_getNearby,               0, FUNCTION_FLAGS),
  JS_FS("getStat",            room_getStat,                 0, FUNCTION_FLAGS),
  JS_FS("getFirst",           room_getFirst,                0, FUNCTION_FLAGS),
  JS_FS("unitInRoom",         room_unitInRoom,              1, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass room_class{
    "Room",                                 // name
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
                    room_ctor,              // construct
                    nullptr)                // trace
};
