#include "JSExits.h"

EMPTY_CTOR(exit)

void exit_finalize(JSFreeOp*, JSObject* obj) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);
  delete pExit;
}

JSAPI_PROP(exit_type) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);

  vp.set(JSVAL_VOID);

  if (!pExit) return JS_TRUE;

  vp.setInt32(pExit->type);
  return JS_TRUE;
}

JSAPI_PROP(exit_tileid) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);

  vp.set(JSVAL_VOID);

  if (!pExit) return JS_TRUE;

  vp.setInt32(pExit->tileid);
  return JS_TRUE;
}

JSAPI_PROP(exit_level) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);

  vp.set(JSVAL_VOID);

  if (!pExit) return JS_TRUE;

  vp.setInt32(pExit->level);
  return JS_TRUE;
}

JSAPI_PROP(exit_x) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);

  vp.set(JSVAL_VOID);

  if (!pExit) return JS_TRUE;

  vp.setInt32(pExit->x);
  return JS_TRUE;
}

JSAPI_PROP(exit_y) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);

  vp.set(JSVAL_VOID);

  if (!pExit) return JS_TRUE;

  vp.setInt32(pExit->y);
  return JS_TRUE;
}

JSAPI_PROP(exit_target) {
  myExit* pExit = (myExit*)JS_GetPrivate(obj);

  vp.set(JSVAL_VOID);

  if (!pExit) return JS_TRUE;

  vp.setInt32(pExit->id);
  return JS_TRUE;
}
