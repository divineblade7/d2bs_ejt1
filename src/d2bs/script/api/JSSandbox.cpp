#include "d2bs/script/api/JSSandbox.h"

#include "d2bs/engine.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/utils/Helpers.h"

JSAPI_FUNC(sandbox_ctor) {
  sandbox* box = new sandbox;  // leaked?
  box->runtime = JS_NewRuntime(Vars.dwMemUsage, JS_NO_HELPER_THREADS);
  box->context = JS_NewContext(box->runtime, 0x2000);
  if (!box->context) {
    delete box;
    return JS_TRUE;
  }
  box->innerObj = JS_NewObject(box->context, &global_obj, NULL, NULL);
  if (!box->innerObj) {
    JS_DestroyContext(box->context);
    delete box;
    return JS_TRUE;
  }

  if (JS_InitStandardClasses(box->context, box->innerObj) == JS_FALSE) {
    JS_DestroyContext(box->context);
    delete box;
    return JS_TRUE;
  }

  // TODO: add a default include function for sandboxed scripts
  // how do I do that individually though? :/

  JSObject* res = JS_NewObject(cx, &sandbox_class, NULL, NULL);
  if (JS_AddRoot(cx, &res) == JS_FALSE) {
    JS_DestroyContext(box->context);
    delete box;
    return JS_TRUE;
  }
  if (!res || !JS_DefineFunctions(cx, res, sandbox_methods)) {
    JS_RemoveRoot(cx, &box->innerObj);
    JS_DestroyContext(box->context);
    delete box;
    return JS_TRUE;
  }
  JS_SetPrivate(cx, res, box);
  JS_RemoveRoot(cx, &res);
  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(res));

  return JS_TRUE;
}

JSAPI_PROP(sandbox_addProperty) {
  sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, obj, &sandbox_class, NULL);
  JSContext* cxptr = (!box ? cx : box->context);
  JSObject* ptr = (!box ? obj : box->innerObj);
  jsval ID;
  JS_IdToValue(cx, id, &ID);

  if (JSVAL_IS_INT(ID)) {
    int32 i;
    if (JS_ValueToInt32(cx, ID, &i) == JS_FALSE) return JS_TRUE;
    char name[32];
    _itoa_s(i, name, 32, 10);
    JSBool found;
    if (JS_HasProperty(cxptr, ptr, name, &found) == JS_FALSE) return JS_TRUE;
    if (found) return JS_TRUE;

    return JS_DefineProperty(cxptr, ptr, name, vp.get(), NULL, NULL, JSPROP_ENUMERATE);
  } else if (JSVAL_IS_STRING(ID)) {
    char* name = JS_EncodeStringToUTF8(cx, JSVAL_TO_STRING(ID));
    JSBool found;
    if (JS_HasProperty(cxptr, ptr, name, &found) == JS_FALSE || found) {
      JS_free(cx, name);
      return JS_TRUE;
    }

    JSBool ret = JS_DefineProperty(cxptr, ptr, name, vp.get(), NULL, NULL, JSPROP_ENUMERATE);
    JS_free(cx, name);
    return ret;
  }
  return JS_FALSE;
}

JSAPI_PROP(sandbox_delProperty) {
  sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, obj, &sandbox_class, NULL);
  jsval ID;
  JS_IdToValue(cx, id, &ID);

  if (JSVAL_IS_INT(ID)) {
    int32 i;
    if (JS_ValueToInt32(cx, ID, &i) == JS_FALSE) return JS_TRUE;
    char name[32];
    _itoa_s(i, name, 32, 10);
    if (box && JS_DeleteProperty(box->context, box->innerObj, name)) return JS_TRUE;
  } else if (JSVAL_IS_STRING(ID)) {
    char* name = JS_EncodeStringToUTF8(cx, JSVAL_TO_STRING(ID));
    if (box && JS_DeleteProperty(box->context, box->innerObj, name)) {
      JS_free(cx, name);
      return JS_TRUE;
    }
    JS_free(cx, name);
  }
  return JS_FALSE;
}

JSAPI_PROP(sandbox_getProperty) {
  sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, obj, &sandbox_class, NULL);
  jsval ID;
  JS_IdToValue(cx, id, &ID);

  if (JSVAL_IS_INT(ID)) {
    int32 i;
    if (JS_ValueToInt32(cx, ID, &i) == JS_FALSE) return JS_TRUE;
    char name[32];
    _itoa_s(i, name, 32, 10);
    vp.set(JSVAL_VOID);
    if (box && JS_LookupProperty(box->context, box->innerObj, name, vp.address())) return JS_TRUE;
    if (JSVAL_IS_VOID(vp.get()) && JS_LookupProperty(cx, obj, name, vp.address())) return JS_TRUE;
  } else if (JSVAL_IS_STRING(ID)) {
    char* name = JS_EncodeStringToUTF8(cx, JSVAL_TO_STRING(ID));
    vp.set(JSVAL_VOID);
    if (box && (JS_LookupProperty(box->context, box->innerObj, name, vp.address()))) {
      JS_free(cx, name);
      return JS_TRUE;
    }
    if (JSVAL_IS_VOID(vp.get()) && JS_LookupProperty(cx, obj, name, vp.address())) {
      JS_free(cx, name);
      return JS_TRUE;
    }
    JS_free(cx, name);
  }
  return JS_FALSE;
}

JSAPI_STRICT_PROP(sandbox_setProperty) {
  sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, obj, &sandbox_class, NULL);
  jsval ID;
  JS_IdToValue(cx, id, &ID);

  if (JSVAL_IS_INT(ID)) {
    int32 i;
    if (JS_ValueToInt32(cx, ID, &i) == JS_FALSE) return JS_TRUE;
    char name[32];
    _itoa_s(i, name, 32, 10);
    if (box)
      if (JS_SetProperty(box->context, box->innerObj, name, vp.address())) return JS_TRUE;
  } else if (JSVAL_IS_STRING(ID)) {
    char* name = JS_EncodeStringToUTF8(cx, JSVAL_TO_STRING(ID));
    if (box && JS_SetProperty(box->context, box->innerObj, name, vp.address())) {
      JS_free(cx, name);
      return JS_TRUE;
    }
    JS_free(cx, name);
  }
  return JS_FALSE;
}

void sandbox_finalize(JSFreeOp*, JSObject* obj) {
  sandbox* box = (sandbox*)JS_GetPrivate(obj);
  if (box) {
    // bob1.8.8		JS_SetContextThread(box->context);
    JS_DestroyContext(box->context);
    delete box;
  }
}

JSAPI_FUNC(sandbox_eval) {
  if (argc > 0 && JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
    sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &sandbox_class, NULL);
    if (!box) THROW_ERROR(cx, "Invalid execution object!");
    char* code = JS_EncodeStringToUTF8(cx, JSVAL_TO_STRING(JS_ARGV(cx, vp)[0]));
    jsval result;
    if (JS_BufferIsCompilableUnit(box->context, box->innerObj, code, strlen(code)) &&
        JS_EvaluateScript(box->context, box->innerObj, code, strlen(code), "sandbox", 0, &result))
      JS_SET_RVAL(cx, vp, result);
  } else
    THROW_ERROR(cx, "Invalid parameter, string expected");
  return JS_TRUE;
}

JSAPI_FUNC(sandbox_include) {
  JS_SET_RVAL(cx, vp, JSVAL_FALSE);
  if (argc > 0 && JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
    sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &sandbox_class, NULL);
    const wchar_t* file = JS_GetStringCharsZ(cx, JSVAL_TO_STRING(JS_ARGV(cx, vp)[0]));
    if (file && wcslen(file) <= _MAX_FNAME && box) {
      auto path = (Vars.script_dir / "libs" / file).make_preferred().wstring();
      if (box->list.count(std::wstring(file)) == -1) {
        JSScript* tmp = JS_CompileFile(box->context, box->innerObj, path.c_str());
        if (tmp) {
          jsval result;
          if (JS_ExecuteScript(box->context, box->innerObj, tmp, &result)) {
            box->list[file] = true;
            JS_SET_RVAL(cx, vp, result);
          }
          // nolonger needed?
          // JS_DestroyScript(cx, tmp);
        }
        // JS_RemoveScriptRoot(cx, &tmp);
      }
    }
  } else
    THROW_ERROR(cx, "Invalid parameter, file expected");

  return JS_TRUE;
}

JSAPI_FUNC(sandbox_isIncluded) {
  sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &sandbox_class, NULL);
  if (argc > 0 && JSVAL_IS_STRING(JS_ARGV(cx, vp)[0]) && box) {
    const wchar_t* file = JS_GetStringCharsZ(cx, JSVAL_TO_STRING(JS_ARGV(cx, vp)[0]));
    auto path = (Vars.script_dir / "libs" / file).make_preferred().wstring();
    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(!!box->list.count(path)));
  } else
    THROW_ERROR(cx, "Invalid parameter, file expected");
  return JS_TRUE;
}

JSAPI_FUNC(sandbox_clear) {
  // sandbox* box = (sandbox*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &sandbox_class, NULL);
  //  if(box)
  //	JS_ClearScope(cx, box->innerObj);
  return JS_TRUE;
}
