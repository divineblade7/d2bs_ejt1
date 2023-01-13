#pragma once

#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/js32.h"
#include "d2bs/utils/sqlite3.h"

JSAPI_FUNC(sqlite_ctor);
void sqlite_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(sqlite_path);
JSAPI_PROP(sqlite_statements);
JSAPI_PROP(sqlite_isOpen);
JSAPI_PROP(sqlite_lastRowId);
JSAPI_PROP(sqlite_changes);

JSAPI_FUNC(my_sqlite_version);
JSAPI_FUNC(my_sqlite_memusage);

JSAPI_FUNC(sqlite_execute);
JSAPI_FUNC(sqlite_query);
JSAPI_FUNC(sqlite_close);
JSAPI_FUNC(sqlite_open);

// clang-format off
static JSPropertySpec sqlite_props[] = {
  JS_PSG("path",            sqlite_path,          JSPROP_PERMANENT_VAR),
  JS_PSG("statements",      sqlite_statements,    JSPROP_PERMANENT_VAR),
  JS_PSG("isOpen",          sqlite_isOpen,        JSPROP_PERMANENT_VAR),
  JS_PSG("lastRowId",       sqlite_lastRowId,     JSPROP_PERMANENT_VAR),
  JS_PSG("changes",         sqlite_changes,       JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec sqlite_methods[] = {
  JS_FS("execute",          sqlite_execute,       1, FUNCTION_FLAGS),
  JS_FS("query",            sqlite_query,         1, FUNCTION_FLAGS),
  JS_FS("open",             sqlite_open,          0, FUNCTION_FLAGS),
  JS_FS("close",            sqlite_close,         0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass sqlite_db{
    "SQLite",                               // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    sqlite_finalize,        // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    sqlite_ctor,            // construct
                    nullptr)                // trace
};

CLASS_CTOR(sqlite_stmt);
void sqlite_stmt_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(sqlite_stmt_sql);
JSAPI_PROP(sqlite_stmt_ready);

JSAPI_FUNC(sqlite_stmt_getobject);
JSAPI_FUNC(sqlite_stmt_colcount);
JSAPI_FUNC(sqlite_stmt_colval);
JSAPI_FUNC(sqlite_stmt_colname);
JSAPI_FUNC(sqlite_stmt_execute);
JSAPI_FUNC(sqlite_stmt_next);
JSAPI_FUNC(sqlite_stmt_reset);
JSAPI_FUNC(sqlite_stmt_skip);
JSAPI_FUNC(sqlite_stmt_close);
JSAPI_FUNC(sqlite_stmt_bind);

// clang-format off
static JSPropertySpec sqlite_stmt_props[] = {
  JS_PSG("sql",             sqlite_stmt_sql,          JSPROP_PERMANENT_VAR),
  JS_PSG("ready",           sqlite_stmt_ready,        JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec sqlite_stmt_methods[] = {
  JS_FS("getObject",        sqlite_stmt_getobject,    0, FUNCTION_FLAGS),
  JS_FS("getColumnCount",   sqlite_stmt_colcount,     0, FUNCTION_FLAGS),
  JS_FS("getColumnValue",   sqlite_stmt_colval,       1, FUNCTION_FLAGS),
  JS_FS("getColumnName",    sqlite_stmt_colname,      1, FUNCTION_FLAGS),
  JS_FS("go",               sqlite_stmt_execute,      0, FUNCTION_FLAGS),
  JS_FS("next",             sqlite_stmt_next,         0, FUNCTION_FLAGS),
  JS_FS("skip",             sqlite_stmt_skip,         1, FUNCTION_FLAGS),
  JS_FS("reset",            sqlite_stmt_reset,        0, FUNCTION_FLAGS),
  JS_FS("close",            sqlite_stmt_close,        0, FUNCTION_FLAGS),
  JS_FS("bind",             sqlite_stmt_bind,         2, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass sqlite_stmt{
    "DBStatement",                          // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    sqlite_stmt_finalize,   // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    sqlite_stmt_ctor,       // construct
                    nullptr)                // trace
};
