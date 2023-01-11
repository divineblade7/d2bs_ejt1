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
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Script* iterp = (Script*)JS_GetInstancePrivate(cx, self, &script_class, NULL);
  auto lock = sScriptEngine->lock_script_list("scrip.getNext");

  auto& scripts = sScriptEngine->scripts();
  for (ScriptMap::iterator it = scripts.begin(); it != scripts.end(); it++) {
    if (it->second == iterp) {
      it++;
      if (it == scripts.end()) {
        break;
      }
      iterp = it->second;
      JS_SetPrivate(self, iterp);
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
      args.rval().setBoolean(true);
      return JS_TRUE;
    }
  }

  args.rval().setUndefined();
  return JS_TRUE;
}

JSAPI_FUNC(script_stop) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS_SET_RVAL(cx, vp, JSVAL_NULL);
  auto self = args.thisv().toObjectOrNull();
  Script* script = (Script*)JS_GetInstancePrivate(cx, self, &script_class, NULL);
  if (script->is_running()) {
    script->stop();
  }

  args.rval().setNull();
  return JS_TRUE;
}

JSAPI_FUNC(script_pause) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Script* script = (Script*)JS_GetInstancePrivate(cx, self, &script_class, NULL);

  if (script->is_running()) {
    script->pause();
  }

  args.rval().setNull();
  return JS_TRUE;
}

JSAPI_FUNC(script_resume) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Script* script = (Script*)JS_GetInstancePrivate(cx, self, &script_class, NULL);

  if (script->is_paused()) {
    script->resume();
  }

  args.rval().setNull();
  return JS_TRUE;
}

JSAPI_FUNC(script_send) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNull();

  auto self = args.thisv().toObjectOrNull();
  Script* script = (Script*)JS_GetInstancePrivate(cx, self, &script_class, NULL);
  if (!script || !script->is_running()) {
    return JS_TRUE;
  }

  std::vector<std::shared_ptr<JSAutoStructuredCloneBuffer>> argv;

  for (uint i = 0; i < argc; ++i) {
    argv.push_back(std::make_shared<JSAutoStructuredCloneBuffer>());
    argv.back()->write(cx, args[i]);
  }

  auto evt = std::make_shared<BroadcastEvent>(script, argv);
  script->FireEvent(evt);

  return JS_TRUE;
}

JSAPI_FUNC(script_join) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Script* script = (Script*)JS_GetInstancePrivate(cx, self, &script_class, NULL);

  script->join();

  args.rval().setNull();
  return JS_TRUE;
}

JSAPI_FUNC(my_getScript) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNull();

  Script* iterp = NULL;
  if (args.length() == 1 && args[0].isBoolean() && args[0].toBoolean() == JS_TRUE) {
    iterp = (Script*)JS_GetContextPrivate(cx);
  } else if (args.length() == 1 && args[0].isInt32()) {
    // loop over the Scripts in ScriptEngine and find the one with the right threadid
    DWORD tid = (DWORD)args[0].toInt32();
    FindHelper argv = {tid, NULL, NULL};
    sScriptEngine->for_each(FindScriptByTid, &argv);
    if (argv.script != NULL)
      iterp = argv.script;
    else
      return JS_TRUE;
  } else if (args.length() == 1 && args[0].isString()) {
    wchar_t* name = _wcsdup(JS_GetStringCharsZ(cx, args[0].toString()));
    if (name) StringReplace(name, L'/', L'\\', wcslen(name));
    FindHelper argv = {0, name, NULL};
    sScriptEngine->for_each(FindScriptByName, &argv);
    free(name);
    if (argv.script != NULL) {
      iterp = argv.script;
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
  
  args.rval().setObjectOrNull(res);
  return JS_TRUE;
}

JSAPI_FUNC(my_getScripts) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  DWORD dwArrayCount = NULL;

  JS::RootedObject pReturnArray(cx, JS_NewArrayObject(cx, 0, NULL));
  JSAutoRequest r(cx);
  auto lock = sScriptEngine->lock_script_list("getScripts");

  auto& scripts = sScriptEngine->scripts();
  for (const auto& [_, script] : scripts) {
    JSObject* res = BuildObject(cx, &script_class, script_methods, script_props, script);
    jsval a = JS::ObjectOrNullValue(res);
    JS_SetElement(cx, pReturnArray, dwArrayCount, &a);
    dwArrayCount++;
  }

  args.rval().setObjectOrNull(pReturnArray);
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
