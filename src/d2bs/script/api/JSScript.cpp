#include "d2bs/script/api/JSScript.h"

#include "d2bs/script/Script.h"
#include "d2bs/script/ScriptEngine.h"
#include "d2bs/utils/Helpers.h"

EMPTY_CTOR(script)

struct FindHelper {
  DWORD tid;
  wchar_t* name;
  Script* script;
};

bool __fastcall FindScriptByTid(Script* script, FindHelper* helper);
bool __fastcall FindScriptByName(Script* script, FindHelper* helper);

JSAPI_PROP(script_getProperty) {
  Script* script = (Script*)JS_GetInstancePrivate(cx, obj, &script_class, NULL);

  // TODO: make this check stronger
  if (!script) return JS_TRUE;
  jsval ID;
  JS_IdToValue(cx, id, &ID);

  switch (JSVAL_TO_INT(ID)) {
    case SCRIPT_FILENAME:
      vp.setString(JS_InternUCString(cx, script->filename_short()));
      break;
    case SCRIPT_GAMETYPE:
      vp.setBoolean(script->type() == ScriptType::InGame ? false : true);
      break;
    case SCRIPT_RUNNING:
      vp.setBoolean(script->is_running());
      break;
    case SCRIPT_THREADID:
      vp.setInt32(script->thread_id());
      break;
    case SCRIPT_MEMORY:
      vp.setInt32(JS_GetGCParameter(script->runtime(), JSGC_BYTES));
      break;
    default:
      break;
  }

  return JS_TRUE;
}

JSAPI_FUNC(script_getNext) {
  Script* iterp = (Script*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &script_class, NULL);
  auto lock = sScriptEngine->lock_script_list("scrip.getNext");

  auto& scripts = sScriptEngine->scripts();
  for (ScriptMap::iterator it = scripts.begin(); it != scripts.end(); it++) {
    if (it->second == iterp) {
      it++;
      if (it == scripts.end()) {
        break;
      }
      iterp = it->second;
      JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), iterp);
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
      return JS_TRUE;
    }
  }

  JS_SET_RVAL(cx, vp, JSVAL_VOID);
  return JS_TRUE;
}

JSAPI_FUNC(script_stop) {
  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  Script* script = (Script*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &script_class, NULL);
  if (script->is_running()) {
    script->stop();
  }

  return JS_TRUE;
}

JSAPI_FUNC(script_pause) {
  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  Script* script = (Script*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &script_class, NULL);

  if (script->is_running()) {
    script->pause();
  }

  return JS_TRUE;
}

JSAPI_FUNC(script_resume) {
  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  Script* script = (Script*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &script_class, NULL);

  if (script->is_paused()) {
    script->resume();
  }

  return JS_TRUE;
}

JSAPI_FUNC(script_send) {
  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  Script* script = (Script*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &script_class, NULL);
  if (!script || !script->is_running()) {
    return JS_TRUE;
  }

  std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> args;

  for (uint i = 0; i < argc; ++i) {
    args.push_back(std::make_shared<JSAutoStructuredCloneBuffer>());
    args.back()->write(cx, JS_ARGV(cx, vp)[i]);
  }

  auto evt = std::make_shared<BroadcastEvent>(script, args);
  script->FireEvent(evt);

  return JS_TRUE;
}

JSAPI_FUNC(script_join) {
  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  Script* script = (Script*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &script_class, NULL);

  script->join();

  return JS_TRUE;
}

JSAPI_FUNC(my_getScript) {
  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  Script* iterp = NULL;
  if (argc == 1 && JSVAL_IS_BOOLEAN(JS_ARGV(cx, vp)[0]) && JSVAL_TO_BOOLEAN(JS_ARGV(cx, vp)[0]) == JS_TRUE) {
    iterp = (Script*)JS_GetContextPrivate(cx);
  } else if (argc == 1 && JSVAL_IS_INT(JS_ARGV(cx, vp)[0])) {
    // loop over the Scripts in ScriptEngine and find the one with the right threadid
    DWORD tid = (DWORD)JSVAL_TO_INT(JS_ARGV(cx, vp)[0]);
    FindHelper args = {tid, NULL, NULL};
    sScriptEngine->for_each(FindScriptByTid, &args);
    if (args.script != NULL)
      iterp = args.script;
    else
      return JS_TRUE;
  } else if (argc == 1 && JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
    wchar_t* name = _wcsdup(JS_GetStringCharsZ(cx, JSVAL_TO_STRING(JS_ARGV(cx, vp)[0])));
    if (name) StringReplace(name, L'/', L'\\', wcslen(name));
    FindHelper args = {0, name, NULL};
    sScriptEngine->for_each(FindScriptByName, &args);
    free(name);
    if (args.script != NULL) {
      iterp = args.script;
    } else {
      return JS_TRUE;
    }
  } else {
    auto lock = sScriptEngine->lock_script_list("getScript");
    if (!sScriptEngine->scripts().empty()) {
      iterp = sScriptEngine->scripts().begin()->second;
    }

    if (iterp == NULL) {
      return JS_TRUE;
    }
  }

  JSObject* res = BuildObject(cx, &script_class, script_methods, script_props, iterp);

  if (!res) THROW_ERROR(cx, "Failed to build the script object");
  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(res));

  return JS_TRUE;
}
JSAPI_FUNC(my_getScripts) {
  DWORD dwArrayCount = NULL;

  JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));
  JS_BeginRequest(cx);
  auto lock = sScriptEngine->lock_script_list("getScripts");

  auto& scripts = sScriptEngine->scripts();
  for (const auto& [_, script] : scripts) {
    JSObject* res = BuildObject(cx, &script_class, script_methods, script_props, script);
    jsval a = OBJECT_TO_JSVAL(res);
    JS_SetElement(cx, pReturnArray, dwArrayCount, &a);
    dwArrayCount++;
  }

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(pReturnArray));
  JS_EndRequest(cx);
  return JS_TRUE;
}
bool __fastcall FindScriptByName(Script* script, FindHelper* helper) {
  const wchar_t* fname = script->filename_short();
  if (_wcsicmp(fname, helper->name) == 0) {
    helper->script = script;
    return false;
  }
  return true;
}

bool __fastcall FindScriptByTid(Script* script, FindHelper* helper) {
  if (script->thread_id() == helper->tid) {
    helper->script = script;
    return false;
  }
  return true;
}
