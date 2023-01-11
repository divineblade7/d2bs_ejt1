#pragma once

#ifndef XP_WIN
#define XP_WIN
#endif

// Disable all warnings emitted from mozjs
// it is a dependency so we dont care. ~ ejt
#pragma warning(push, 0)
#include "jsapi.h"
#include "jsfriendapi.h"
#pragma warning(pop)

#include <string>

typedef unsigned int uint;
typedef uint32_t uint32;
typedef int32_t int32;
typedef double jsdouble;
typedef int32_t jsint;
typedef uint32_t jsuint;
typedef uint16_t uint16;

// Deprecate, removed in ESR45+
#define NUM(x) #x
#define NAME(line, v) (__FILE__ ":" NUM(line) " -> " #v)
#define JS_AddValueRoot(cx, vp) JS_AddNamedValueRoot((cx), (vp), NAME(__LINE__, vp))
#define JS_AddObjectRoot(cx, vp) JS_AddNamedObjectRoot((cx), (vp), NAME(__LINE__, vp))

JSBool JSVAL_IS_OBJECT(jsval v);

// IMPORTANT: Ordering is critical here! If your object has a
// defined prototype, _THAT PROTOTYPE MUST BE LISTED ABOVE IT_
struct JSClassSpec {
  JSClass* classp;
  JSClass* proto;
  JSNative ctor;
  uint argc;
  JSFunctionSpec* methods;
  JSPropertySpec* properties;
  JSFunctionSpec* static_methods;
  JSPropertySpec* static_properties;
};

struct JSModuleSpec {
  const wchar_t* name;
  JSClassSpec* classes;
  JSFunctionSpec* static_methods;
  JSPropertySpec* static_properties;
};

JSObject* BuildObject(JSContext* cx, JSClass* classp = NULL, JSFunctionSpec* funcs = NULL, JSPropertySpec* props = NULL,
                      void* priv = NULL, JSObject* proto = NULL, JSObject* parent = NULL);
JSScript* JS_CompileFile(JSContext* cx, JSObject* globalObject, std::wstring fileName);

#define JSVAL_IS_FUNCTION(cx, var) (!JSVAL_IS_PRIMITIVE(var) && JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(var)))

#define JSPROP_PERMANENT_VAR (JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT)
#define JSPROP_STATIC_VAR (JSPROP_ENUMERATE | JSPROP_PERMANENT)
#define JSPROP_STATIC JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY

#define FUNCTION_FLAGS JSFUN_STUB_GSOPS

#define THROW_ERROR(cx, msg) \
  {                          \
    JS_ReportError(cx, msg); \
    return JS_FALSE;         \
  }

#define THROW_WARNING(cx, vp, msg)    \
  {                                   \
    JS_ReportWarning(cx, msg);        \
    JS_SET_RVAL(cx, vp, JSVAL_FALSE); \
    return JS_TRUE;                   \
  }

#define CLASS_CTOR(name) JSBool name##_ctor(JSContext* cx, uint argc, jsval* vp)

#define EMPTY_CTOR(name) \
  JSBool name##_ctor(JSContext* cx, uint, jsval*) { THROW_ERROR(cx, "Invalid Operation"); }

#define JSAPI_FUNC(name) \
  JSBool name##([[maybe_unused]] JSContext * cx, [[maybe_unused]] uint argc, [[maybe_unused]] jsval * vp)

#define JSAPI_PROP(name)                                                                                              \
  JSBool name##([[maybe_unused]] JSContext * cx, [[maybe_unused]] JSHandleObject obj, [[maybe_unused]] JSHandleId id, \
                [[maybe_unused]] JSMutableHandleValue vp)
#define JSAPI_STRICT_PROP(name)                                                                                       \
  JSBool name##([[maybe_unused]] JSContext * cx, [[maybe_unused]] JSHandleObject obj, [[maybe_unused]] JSHandleId id, \
                [[maybe_unused]] JSBool strict, [[maybe_unused]] JSMutableHandleValue vp)

#define JS_PS(name, id, flags, getter, setter) \
  { name, id, flags, getter, setter }

#define JS_PS_END JS_PS(0, 0, 0, 0, 0)

#define JSCLASS_SPEC(add, del, get, set, enumerate, resolve, convert, finalize, ctor) \
  add, del, get, set, enumerate, resolve, convert, finalize, NULL, NULL, NULL, ctor, NULL
