#pragma once

#include "d2bs/script/js32.h"

#include <windows.h>

CLASS_CTOR(area);

void area_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(area_getProperty);

JSAPI_FUNC(my_getArea);

class ApiArea {
 public:
  ApiArea() = default;
  ~ApiArea() = default;

  DWORD AreaId;
  DWORD Exits;
  JSObject* ExitArray;
};

enum area_tinyid { AUNIT_EXITS, AUNIT_NAME, AUNIT_X, AUNIT_XSIZE, AUNIT_Y, AUNIT_YSIZE, AUNIT_ID };

static JSPropertySpec area_props[] = {
    {"exits", AUNIT_EXITS, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {"name", AUNIT_NAME, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {"x", AUNIT_X, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {"xsize", AUNIT_XSIZE, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {"y", AUNIT_Y, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {"ysize", AUNIT_YSIZE, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {"id", AUNIT_ID, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(area_getProperty), JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}};

static JSClass area_class = {"Area", JSCLASS_HAS_PRIVATE,
                             JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                          JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, area_finalize, area_ctor)};
