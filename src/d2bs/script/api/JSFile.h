#pragma once

#include "d2bs/script/js32.h"

CLASS_CTOR(file);
void file_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(file_readable);
JSAPI_PROP(file_writeable);
JSAPI_PROP(file_seekable);
JSAPI_PROP(file_mode);
JSAPI_PROP(file_binaryMode);
JSAPI_PROP(file_length);
JSAPI_PROP(file_path);
JSAPI_PROP(file_position);
JSAPI_PROP(file_eof);
JSAPI_PROP(file_accessed);
JSAPI_PROP(file_created);
JSAPI_PROP(file_modified);
JSAPI_PROP(file_autoflush);
JSAPI_STRICT_PROP(file_autoflush_setter);

JSAPI_FUNC(file_open);
JSAPI_FUNC(file_close);
JSAPI_FUNC(file_reopen);
JSAPI_FUNC(file_read);
JSAPI_FUNC(file_readLine);
JSAPI_FUNC(file_readAllLines);
JSAPI_FUNC(file_readAll);
JSAPI_FUNC(file_write);
JSAPI_FUNC(file_seek);
JSAPI_FUNC(file_flush);
JSAPI_FUNC(file_reset);
JSAPI_FUNC(file_end);

// clang-format off
static JSPropertySpec file_props[] = {
    JS_PSG("readable",      file_readable,                                JSPROP_PERMANENT_VAR),
    JS_PSG("writeable",     file_writeable,                               JSPROP_PERMANENT_VAR),
    JS_PSG("seekable",      file_seekable,                                JSPROP_PERMANENT_VAR),
    JS_PSG("mode",          file_mode,                                    JSPROP_PERMANENT_VAR),
    JS_PSG("binaryMode",    file_binaryMode,                              JSPROP_PERMANENT_VAR),
    JS_PSG("length",        file_length,                                  JSPROP_PERMANENT_VAR),
    JS_PSG("path",          file_path,                                    JSPROP_PERMANENT_VAR),
    JS_PSG("position",      file_position,                                JSPROP_PERMANENT_VAR),
    JS_PSG("eof",           file_eof,                                     JSPROP_PERMANENT_VAR),
    JS_PSG("accessed",      file_accessed,                                JSPROP_PERMANENT_VAR),
    JS_PSG("created",       file_created,                                 JSPROP_PERMANENT_VAR),
    JS_PSG("modified",      file_modified,                                JSPROP_PERMANENT_VAR),
    JS_PSGS("autoflush",    file_autoflush,     file_autoflush_setter,    JSPROP_STATIC_VAR),
    JS_PS_END
};

static JSFunctionSpec file_methods[] = {
  JS_FN("close",            file_close,           0,    FUNCTION_FLAGS),
  JS_FN("reopen",           file_reopen,          0,    FUNCTION_FLAGS),
  JS_FN("read",             file_read,            1,    FUNCTION_FLAGS),
  JS_FN("readLine",         file_readLine,        0,    FUNCTION_FLAGS),
  JS_FN("readAllLines",     file_readAllLines,    0,    FUNCTION_FLAGS),
  JS_FN("readAll",          file_readAll,         0,    FUNCTION_FLAGS),
  JS_FN("write",            file_write,           1,    FUNCTION_FLAGS),
  JS_FN("seek",             file_seek,            1,    FUNCTION_FLAGS),
  JS_FN("flush",            file_flush,           0,    FUNCTION_FLAGS),
  JS_FN("reset",            file_reset,           0,    FUNCTION_FLAGS),
  JS_FN("end",              file_end,             0,    FUNCTION_FLAGS),
  JS_FS_END
};

static JSFunctionSpec file_s_methods[] = {
  JS_FN("open",             file_open,            2,    FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass file_class{
    "File",                                 // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    file_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    file_ctor,              // construct
                    nullptr)                // trace
};
