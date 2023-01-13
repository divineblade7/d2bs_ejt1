#pragma once

#include "d2bs/script/js32.h"

/*********************************************************
                                        Frame Header
**********************************************************/
JSAPI_FUNC(frame_ctor);
void hook_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(frame_x);
JSAPI_STRICT_PROP(frame_x_setter);
JSAPI_PROP(frame_y);
JSAPI_STRICT_PROP(frame_y_setter);
JSAPI_PROP(frame_xsize);
JSAPI_STRICT_PROP(frame_xsize_setter);
JSAPI_PROP(frame_ysize);
JSAPI_STRICT_PROP(frame_ysize_setter);
JSAPI_PROP(frame_visible);
JSAPI_STRICT_PROP(frame_visible_setter);
JSAPI_PROP(frame_align);
JSAPI_STRICT_PROP(frame_align_setter);
JSAPI_PROP(frame_zorder);
JSAPI_STRICT_PROP(frame_zorder_setter);
JSAPI_PROP(frame_click);
JSAPI_STRICT_PROP(frame_click_setter);
JSAPI_PROP(frame_hover);
JSAPI_STRICT_PROP(frame_hover_setter);

JSAPI_FUNC(hook_remove);
JSAPI_PROP(frame_getProperty);
JSAPI_STRICT_PROP(frame_setProperty);

// clang-format off
static JSPropertySpec frame_props[] = {
  JS_PSGS("x",            frame_x,          frame_x_setter,           JSPROP_STATIC_VAR),
  JS_PSGS("y",            frame_y,          frame_y_setter,           JSPROP_STATIC_VAR),
  JS_PSGS("xsize",        frame_xsize,      frame_xsize_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("ysize",        frame_ysize,      frame_ysize_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("visible",      frame_visible,    frame_visible_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("align",        frame_align,      frame_align_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("zorder",       frame_zorder,     frame_zorder_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("click",        frame_click,      frame_click_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("hover",        frame_hover,      frame_hover_setter,       JSPROP_STATIC_VAR),
  JS_PS_END
};

static JSFunctionSpec frame_methods[] = {
  JS_FS("remove",         hook_remove,      0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass frame_class{
    "Frame",                                // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    hook_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    frame_ctor,             // construct
                    nullptr)                // trace
};

/*********************************************************
                                        box Header
**********************************************************/
JSAPI_FUNC(box_ctor);

JSAPI_PROP(box_x);
JSAPI_STRICT_PROP(box_x_setter);
JSAPI_PROP(box_y);
JSAPI_STRICT_PROP(box_y_setter);
JSAPI_PROP(box_xsize);
JSAPI_STRICT_PROP(box_xsize_setter);
JSAPI_PROP(box_ysize);
JSAPI_STRICT_PROP(box_ysize_setter);
JSAPI_PROP(box_visible);
JSAPI_STRICT_PROP(box_visible_setter);
JSAPI_PROP(box_color);
JSAPI_STRICT_PROP(box_color_setter);
JSAPI_PROP(box_opacity);
JSAPI_STRICT_PROP(box_opacity_setter);
JSAPI_PROP(box_align);
JSAPI_STRICT_PROP(box_align_setter);
JSAPI_PROP(box_zorder);
JSAPI_STRICT_PROP(box_zorder_setter);
JSAPI_PROP(box_click);
JSAPI_STRICT_PROP(box_click_setter);
JSAPI_PROP(box_hover);
JSAPI_STRICT_PROP(box_hover_setter);

// clang-format off
static JSPropertySpec box_props[] = {
  JS_PSGS("x",          box_x,          box_x_setter,           JSPROP_STATIC_VAR),
  JS_PSGS("y",          box_y,          box_y_setter,           JSPROP_STATIC_VAR),
  JS_PSGS("xsize",      box_xsize,      box_xsize_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("ysize",      box_ysize,      box_ysize_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("visible",    box_visible,    box_visible_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("color",      box_color,      box_color_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("opacity",    box_opacity,    box_opacity_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("align",      box_align,      box_align_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("zorder",     box_zorder,     box_zorder_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("click",      box_click,      box_click_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("hover",      box_hover,      box_hover_setter,       JSPROP_STATIC_VAR),
  JS_PS_END
};

static JSFunctionSpec box_methods[] = {
  JS_FS("remove",       hook_remove,    0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass box_class{
    "Box",                                  // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    hook_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    box_ctor,               // construct
                    nullptr)                // trace
};

/*********************************************************
                                        Line Header
**********************************************************/
JSAPI_FUNC(line_ctor);

JSAPI_PROP(line_x);
JSAPI_STRICT_PROP(line_x_setter);
JSAPI_PROP(line_y);
JSAPI_STRICT_PROP(line_y_setter);
JSAPI_PROP(line_x2);
JSAPI_STRICT_PROP(line_x2_setter);
JSAPI_PROP(line_y2);
JSAPI_STRICT_PROP(line_y2_setter);
JSAPI_PROP(line_visible);
JSAPI_STRICT_PROP(line_visible_setter);
JSAPI_PROP(line_color);
JSAPI_STRICT_PROP(line_color_setter);
JSAPI_PROP(line_zorder);
JSAPI_STRICT_PROP(line_zorder_setter);
JSAPI_PROP(line_click);
JSAPI_STRICT_PROP(line_click_setter);
JSAPI_PROP(line_hover);
JSAPI_STRICT_PROP(line_hover_setter);

// clang-format off
static JSPropertySpec line_props[] = {
  JS_PSGS("x",          line_x,         line_x_setter,          JSPROP_STATIC_VAR),
  JS_PSGS("y",          line_y,         line_y_setter,          JSPROP_STATIC_VAR),
  JS_PSGS("x2",         line_x2,        line_x2_setter,         JSPROP_STATIC_VAR),
  JS_PSGS("y2",         line_y2,        line_y2_setter,         JSPROP_STATIC_VAR),
  JS_PSGS("visible",    line_visible,   line_visible_setter,    JSPROP_STATIC_VAR),
  JS_PSGS("color",      line_color,     line_color_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("zorder",     line_zorder,    line_zorder_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("click",      line_click,     line_click_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("hover",      line_hover,     line_hover_setter,      JSPROP_STATIC_VAR),
  JS_PS_END
};

static JSFunctionSpec line_methods[] = {
  JS_FS("remove",       hook_remove,    0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass line_class{
    "Line",                                 // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    hook_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    line_ctor,              // construct
                    nullptr)                // trace
};

/*********************************************************
                                        Text Header
**********************************************************/
JSAPI_FUNC(text_ctor);

JSAPI_PROP(text_x);
JSAPI_STRICT_PROP(text_x_setter);
JSAPI_PROP(text_y);
JSAPI_STRICT_PROP(text_y_setter);
JSAPI_PROP(text_font);
JSAPI_STRICT_PROP(text_font_setter);
JSAPI_PROP(text_visible);
JSAPI_STRICT_PROP(text_visible_setter);
JSAPI_PROP(text_color);
JSAPI_STRICT_PROP(text_color_setter);
JSAPI_PROP(text_text);
JSAPI_STRICT_PROP(text_text_setter);
JSAPI_PROP(text_align);
JSAPI_STRICT_PROP(text_align_setter);
JSAPI_PROP(text_zorder);
JSAPI_STRICT_PROP(text_zorder_setter);
JSAPI_PROP(text_click);
JSAPI_STRICT_PROP(text_click_setter);
JSAPI_PROP(text_hover);
JSAPI_STRICT_PROP(text_hover_setter);

// clang-format off
static JSPropertySpec text_props[] = {
  JS_PSGS("x",            text_x,           text_x_setter,          JSPROP_STATIC_VAR),
  JS_PSGS("y",            text_y,           text_y_setter,          JSPROP_STATIC_VAR),
  JS_PSGS("font",         text_font,        text_font_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("visible",      text_visible,     text_visible_setter,    JSPROP_STATIC_VAR),
  JS_PSGS("color",        text_color,       text_color_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("text",         text_text,        text_text_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("align",        text_align,       text_align_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("zorder",       text_zorder,      text_zorder_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("click",        text_click,       text_click_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("hover",        text_hover,       text_hover_setter,      JSPROP_STATIC_VAR),
  JS_PS_END
};

static JSFunctionSpec text_methods[] = {
  JS_FS("remove",         hook_remove,      0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass text_class{
    "Text",                                 // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    hook_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    text_ctor,              // construct
                    nullptr)                // trace
};

/*********************************************************
                                        Image Header
**********************************************************/
JSAPI_FUNC(image_ctor);

JSAPI_PROP(image_x);
JSAPI_STRICT_PROP(image_x_setter);
JSAPI_PROP(image_y);
JSAPI_STRICT_PROP(image_y_setter);
JSAPI_PROP(image_location);
JSAPI_STRICT_PROP(image_location_setter);
JSAPI_PROP(image_visible);
JSAPI_STRICT_PROP(image_visible_setter);
JSAPI_PROP(image_align);
JSAPI_STRICT_PROP(image_align_setter);
JSAPI_PROP(image_zorder);
JSAPI_STRICT_PROP(image_zorder_setter);
JSAPI_PROP(image_click);
JSAPI_STRICT_PROP(image_click_setter);
JSAPI_PROP(image_hover);
JSAPI_STRICT_PROP(image_hover_setter);

JSAPI_FUNC(screenToAutomap);
JSAPI_FUNC(automapToScreen);

// clang-format off
static JSPropertySpec image_props[] = {
  JS_PSGS("x",            image_x,            image_x_setter,           JSPROP_STATIC_VAR),
  JS_PSGS("y",            image_y,            image_y_setter,           JSPROP_STATIC_VAR),
  JS_PSGS("location",     image_location,     image_location_setter,    JSPROP_STATIC_VAR),
  JS_PSGS("visible",      image_visible,      image_visible_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("align",        image_align,        image_align_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("zorder",       image_zorder,       image_zorder_setter,      JSPROP_STATIC_VAR),
  JS_PSGS("click",        image_click,        image_click_setter,       JSPROP_STATIC_VAR),
  JS_PSGS("hover",        image_hover,        image_hover_setter,       JSPROP_STATIC_VAR),
  JS_PS_END
};

static JSFunctionSpec image_methods[] = {
  JS_FS("remove",         hook_remove,        0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass image_class{
    "Image",                                // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    hook_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    image_ctor,             // construct
                    nullptr)                // trace
};
