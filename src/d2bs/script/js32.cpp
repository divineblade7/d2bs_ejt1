#include "d2bs/script/js32.h"

#include "d2bs/new_util/localization.h"

#include <fstream>

JSObject* BuildObject(JSContext* cx, JSClass* classp, JSFunctionSpec* funcs, JSPropertySpec* props, void* priv,
                      JSObject* proto, JSObject* parent) {
  JS_BeginRequest(cx);

  JS::RootedObject obj(cx, JS_NewObject(cx, classp, proto, parent));

  if (obj) {
    // add root to avoid newborn root problem
    // if(JS_AddRoot(&obj) == JS_FALSE)
    // return NULL;
    if (obj && funcs && !JS_DefineFunctions(cx, obj, funcs)) obj = NULL;
    if (obj && props && !JS_DefineProperties(cx, obj, props)) obj = NULL;
    if (obj && priv) JS_SetPrivate(obj, priv);
  }
  JS_EndRequest(cx);
  return obj;
}

JSScript* JS_CompileFile(JSContext* cx, JSObject* globalObject, std::wstring fileName) {
  std::ifstream t(fileName.c_str(), std::ios::binary);
  std::string str;

  int ch1 = t.get();
  int ch2 = t.get();
  int ch3 = t.get();

  t.seekg(0, std::ios::end);
  str.resize(static_cast<size_t>(t.tellg()));
  t.seekg(0, std::ios::beg);

  str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

  if (ch1 == 0xEF && ch2 == 0xBB && ch3 == 0xBF) {  // replace utf8-bom with space
    str[0] = ' ';
    str[1] = ' ';
    str[2] = ' ';
  }

  std::string nFileName = d2bs::util::wide_to_utf8(fileName);

  JS::RootedObject obj(cx, globalObject);
  JS::CompileOptions opts(cx);
  opts.setUTF8(true).setFileAndLine(nFileName.c_str(), 1);
  JSScript* rval = JS::Compile(cx, obj, opts, str.c_str(), str.size());
  JS_AddNamedScriptRoot(cx, &rval, nFileName.c_str());
  JS_RemoveScriptRoot(cx, &rval);
  t.close();

  return rval;
}
