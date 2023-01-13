#include "d2bs/script/api/JSScreenHook.h"

#include "d2bs/core/File.h"
#include "d2bs/core/ScreenHook.h"
#include "d2bs/script/Script.h"

// Function to create a frame which gets called on a "new Frame ()"
// Parameters: x, y, xsize, ysize, alignment, automap, onClick, onHover
JSAPI_FUNC(frame_ctor) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);

  uint32_t x = 0, y = 0, x2 = 0, y2 = 0;
  Align align = Left;
  bool automap = false;
  JS::Value click = JSVAL_VOID, hover = JSVAL_VOID;

  if (args.get(0).isInt32()) x = args[0].toInt32();
  if (args.get(1).isInt32()) y = args[1].toInt32();
  if (args.get(2).isInt32()) x2 = args[2].toInt32();
  if (args.get(3).isInt32()) y2 = args[3].toInt32();
  if (args.get(4).isInt32()) align = (Align)args[4].toInt32();
  if (args.get(5).isBoolean()) automap = args[5].toBoolean();
  if (JSVAL_IS_FUNCTION(cx, args.get(6))) click = args[6];
  if (JSVAL_IS_FUNCTION(cx, args.get(7))) hover = args[7];

  JSObject* hook = BuildObject(cx, &frame_class, frame_methods, frame_props);
  if (!hook) THROW_ERROR(cx, "Failed to create frame object");

  // framehooks don't work out of game -- they just crash
  FrameHook* pFrameHook = new FrameHook(script, hook, x, y, x2, y2, automap, align, IG);

  if (!pFrameHook) THROW_ERROR(cx, "Failed to create framehook");

  JS_SetPrivate(hook, pFrameHook);
  pFrameHook->SetClickHandler(click);
  pFrameHook->SetHoverHandler(hover);

  args.rval().setObjectOrNull(hook);

  return JS_TRUE;
}

void hook_finalize(JSFreeOp*, JSObject* obj) {
  Genhook* hook = (Genhook*)JS_GetPrivate(obj);
  Genhook::EnterGlobalSection();
  if (hook) {
    JS_SetPrivate(obj, NULL);
    delete hook;
  }
  Genhook::LeaveGlobalSection();
}

JSAPI_PROP(frame_x) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setInt32(pFramehook->GetX());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_x_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isInt32()) pFramehook->SetX(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(frame_y) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setInt32(pFramehook->GetY());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_y_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isInt32()) pFramehook->SetY(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(frame_xsize) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setInt32(pFramehook->GetXSize());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_xsize_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isInt32()) pFramehook->SetXSize(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(frame_ysize) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setInt32(pFramehook->GetYSize());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_ysize_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isInt32()) pFramehook->SetYSize(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(frame_visible) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setBoolean(pFramehook->GetIsVisible());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_visible_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isBoolean()) pFramehook->SetIsVisible(!!vp.toBoolean());
  return JS_TRUE;
}

JSAPI_PROP(frame_align) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setInt32(pFramehook->GetAlign());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_align_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isInt32()) pFramehook->SetAlign((Align)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(frame_zorder) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.setInt32(pFramehook->GetZOrder());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_zorder_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  if (vp.isInt32()) pFramehook->SetZOrder((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(frame_click) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.set(pFramehook->GetClickHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_click_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  pFramehook->SetClickHandler(vp.get());
  return JS_TRUE;
}

JSAPI_PROP(frame_hover) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  vp.set(pFramehook->GetHoverHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(frame_hover_setter) {
  FrameHook* pFramehook = (FrameHook*)JS_GetPrivate(obj);
  if (!pFramehook) return JS_TRUE;

  pFramehook->SetHoverHandler(vp.get());
  return JS_TRUE;
}

JSAPI_FUNC(hook_remove) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  Genhook::EnterGlobalSection();
  Genhook* hook = (Genhook*)JS_GetPrivate(self);
  if (hook) {
    // hook->SetIsVisible(false);
    delete hook;
  }

  JS_SetPrivate(self, NULL);
  // JS_ClearScope(cx, obj);
  JS_ValueToObject(cx, JS::UndefinedValue(), &self);
  Genhook::LeaveGlobalSection();

  return JS_TRUE;
}

// Box functions

// Parameters: x, y, xsize, ysize, color, opacity, alignment, automap, onClick, onHover
JSAPI_FUNC(box_ctor) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);

  ScreenhookState state = (script->type() == ScriptType::OutOfGame) ? OOG : IG;
  uint32_t x = 0, y = 0, x2 = 0, y2 = 0;
  ushort color = 0, opacity = 0;
  Align align = Left;
  bool automap = false;
  JS::Value click = JSVAL_VOID, hover = JSVAL_VOID;

  if (args.get(0).isInt32()) x = args[0].toInt32();
  if (args.get(1).isInt32()) y = args[1].toInt32();
  if (args.get(2).isInt32()) x2 = args[2].toInt32();
  if (args.get(3).isInt32()) y2 = args[3].toInt32();
  if (args.get(4).isInt32()) color = (ushort)args[4].toInt32();
  if (args.get(5).isInt32()) opacity = (ushort)args[5].toInt32();
  if (args.get(6).isInt32()) align = (Align)args[6].toInt32();
  if (args.get(7).isBoolean()) automap = args[7].toBoolean();
  if (JSVAL_IS_FUNCTION(cx, args.get(8))) click = args[8];
  if (JSVAL_IS_FUNCTION(cx, args.get(9))) hover = args[9];

  JSObject* hook = BuildObject(cx, &box_class, box_methods, box_props);
  if (!hook) THROW_ERROR(cx, "Failed to create box object");

  BoxHook* pBoxHook = new BoxHook(script, hook, x, y, x2, y2, color, opacity, automap, align, state);

  if (!pBoxHook) THROW_ERROR(cx, "Unable to initalize a box class.");

  JS_SetPrivate(hook, pBoxHook);
  pBoxHook->SetClickHandler(click);
  pBoxHook->SetHoverHandler(hover);

  args.rval().setObjectOrNull(hook);

  return JS_TRUE;
}

JSAPI_PROP(box_x) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetX());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_x_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetX(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_y) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetY());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_y_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetY(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_xsize) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetXSize());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_xsize_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetXSize(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_ysize) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetYSize());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_ysize_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetYSize(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_visible) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setBoolean(pBoxHook->GetIsVisible());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_visible_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isBoolean()) pBoxHook->SetIsVisible(!!vp.toBoolean());
  return JS_TRUE;
}

JSAPI_PROP(box_color) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetColor());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_color_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetColor((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_opacity) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetOpacity());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_opacity_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetOpacity((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_align) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetAlign());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_align_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetAlign((Align)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_zorder) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.setInt32(pBoxHook->GetZOrder());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_zorder_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  if (vp.isInt32()) pBoxHook->SetZOrder((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(box_click) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.set(pBoxHook->GetClickHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_click_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  pBoxHook->SetClickHandler(vp.get());
  return JS_TRUE;
}

JSAPI_PROP(box_hover) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  vp.set(pBoxHook->GetHoverHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(box_hover_setter) {
  BoxHook* pBoxHook = (BoxHook*)JS_GetPrivate(obj);
  if (!pBoxHook) return JS_TRUE;

  pBoxHook->SetHoverHandler(vp.get());
  return JS_TRUE;
}

// Line functions

// Parameters: x, y, x2, y2, color, automap, click, hover
JSAPI_FUNC(line_ctor) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);

  ScreenhookState state = (script->type() == ScriptType::OutOfGame) ? OOG : IG;
  int x = 0, y = 0, x2 = 0, y2 = 0;
  ushort color = 0;
  bool automap = false;
  JS::Value click = JSVAL_VOID, hover = JSVAL_VOID;

  if (args.get(0).isInt32()) x = args[0].toInt32();
  if (args.get(1).isInt32()) y = args[1].toInt32();
  if (args.get(2).isInt32()) x2 = args[2].toInt32();
  if (args.get(3).isInt32()) y2 = args[3].toInt32();
  if (args.get(4).isInt32()) color = (ushort)args[4].toInt32();
  if (args.get(5).isBoolean()) automap = args[5].toBoolean();
  if (JSVAL_IS_FUNCTION(cx, args.get(6))) click = args[6];
  if (JSVAL_IS_FUNCTION(cx, args.get(7))) hover = args[7];

  JSObject* hook = BuildObject(cx, &line_class, line_methods, line_props);
  if (!hook) THROW_ERROR(cx, "Failed to create line object");

  LineHook* pLineHook = new LineHook(script, hook, x, y, x2, y2, color, automap, Left, state);

  if (!pLineHook) THROW_ERROR(cx, "Unable to initalize a line class.");

  JS_SetPrivate(hook, pLineHook);
  pLineHook->SetClickHandler(click);
  pLineHook->SetHoverHandler(hover);

  args.rval().setObjectOrNull(hook);

  return JS_TRUE;
}

JSAPI_PROP(line_x) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setInt32(pLineHook->GetX());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_x_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isInt32()) pLineHook->SetX(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(line_y) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setInt32(pLineHook->GetY());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_y_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isInt32()) pLineHook->SetY(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(line_x2) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setInt32(pLineHook->GetX2());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_x2_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isInt32()) pLineHook->SetX2(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(line_y2) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setInt32(pLineHook->GetY2());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_y2_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isInt32()) pLineHook->SetY2(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(line_visible) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setBoolean(pLineHook->GetIsVisible());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_visible_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isBoolean()) pLineHook->SetIsVisible(!!vp.toBoolean());
  return JS_TRUE;
}

JSAPI_PROP(line_color) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setInt32(pLineHook->GetColor());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_color_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isInt32()) pLineHook->SetColor((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(line_zorder) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.setInt32(pLineHook->GetZOrder());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_zorder_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  if (vp.isInt32()) pLineHook->SetZOrder((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(line_click) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.set(pLineHook->GetClickHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_click_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  pLineHook->SetClickHandler(vp.get());
  return JS_TRUE;
}

JSAPI_PROP(line_hover) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  vp.set(pLineHook->GetHoverHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(line_hover_setter) {
  LineHook* pLineHook = (LineHook*)JS_GetPrivate(obj);
  if (!pLineHook) return JS_TRUE;

  pLineHook->SetHoverHandler(vp.get());
  return JS_TRUE;
}

// Function to create a text which gets called on a "new text ()"

// Parameters: text, x, y, color, font, align, automap, onHover, onText
JSAPI_FUNC(text_ctor) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);

  ScreenhookState state = (script->type() == ScriptType::OutOfGame) ? OOG : IG;
  uint32_t x = 0, y = 0;
  ushort color = 0, font = 0;
  Align align = Left;
  bool automap = false;
  JS::Value click = JSVAL_VOID, hover = JSVAL_VOID;
  const wchar_t* szText = L"";

  if (args.get(0).isString()) szText = JS_GetStringCharsZ(cx, args[0].toString());
  if (!szText) return JS_TRUE;
  if (args.get(1).isInt32()) x = args[1].toInt32();
  if (args.get(2).isInt32()) y = args[2].toInt32();
  if (args.get(3).isInt32()) color = (ushort)args[3].toInt32();
  if (args.get(4).isInt32()) font = (ushort)args[4].toInt32();
  if (args.get(5).isInt32()) align = (Align)args[5].toInt32();
  if (args.get(6).isBoolean()) automap = args[6].toBoolean();
  if (JSVAL_IS_FUNCTION(cx, args.get(7))) click = args[7];
  if (JSVAL_IS_FUNCTION(cx, args.get(8))) hover = args[8];

  JSObject* hook = BuildObject(cx, &text_class, text_methods, text_props);
  if (!hook) THROW_ERROR(cx, "Failed to create text object");

  TextHook* pTextHook = new TextHook(script, hook, szText, x, y, font, color, automap, align, state);

  if (!pTextHook) THROW_ERROR(cx, "Failed to create texthook");

  JS_SetPrivate(hook, pTextHook);
  pTextHook->SetClickHandler(click);
  pTextHook->SetHoverHandler(hover);

  args.rval().setObjectOrNull(hook);

  return JS_TRUE;
}

JSAPI_PROP(text_x) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setInt32(pTextHook->GetX());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_x_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isInt32()) pTextHook->SetX(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(text_y) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setInt32(pTextHook->GetY());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_y_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isInt32()) pTextHook->SetY(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(text_font) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setInt32(pTextHook->GetFont());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_font_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isInt32()) pTextHook->SetFont((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(text_visible) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setBoolean(pTextHook->GetIsVisible());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_visible_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isBoolean()) pTextHook->SetIsVisible(!!vp.toBoolean());
  return JS_TRUE;
}

JSAPI_PROP(text_color) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setInt32(pTextHook->GetColor());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_color_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isInt32()) pTextHook->SetColor((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(text_text) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setString(JS_InternUCString(cx, pTextHook->GetText()));
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_text_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isString()) {
    const wchar_t* pText = JS_GetStringCharsZ(cx, vp.toString());
    if (!pText) return JS_TRUE;
    pTextHook->SetText(pText);
  }
  return JS_TRUE;
}

JSAPI_PROP(text_align) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setInt32(pTextHook->GetAlign());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_align_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isInt32()) pTextHook->SetAlign((Align)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(text_zorder) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.setInt32(pTextHook->GetZOrder());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_zorder_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  if (vp.isInt32()) pTextHook->SetZOrder((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(text_click) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.set(pTextHook->GetClickHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_click_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  pTextHook->SetClickHandler(vp.get());
  return JS_TRUE;
}

JSAPI_PROP(text_hover) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  vp.set(pTextHook->GetHoverHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(text_hover_setter) {
  TextHook* pTextHook = (TextHook*)JS_GetPrivate(obj);
  if (!pTextHook) return JS_TRUE;

  pTextHook->SetHoverHandler(vp.get());
  return JS_TRUE;
}

// Function to create a image which gets called on a "new Image ()"

// Parameters: image, x, y, color, align, automap, onHover, onimage
JSAPI_FUNC(image_ctor) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  Script* script = (Script*)JS_GetContextPrivate(cx);

  ScreenhookState state = (script->type() == ScriptType::OutOfGame) ? OOG : IG;
  uint32_t x = 0, y = 0;
  ushort color = 0;
  Align align = Left;
  bool automap = false;
  JS::Value click = JSVAL_VOID, hover = JSVAL_VOID;
  const wchar_t* szText = L"";
  wchar_t path[_MAX_FNAME + _MAX_PATH];

  if (args.get(0).isString()) szText = JS_GetStringCharsZ(cx, args[0].toString());
  if (!szText) return JS_TRUE;
  if (args.get(1).isInt32()) x = args[1].toInt32();
  if (args.get(2).isInt32()) y = args[2].toInt32();
  if (args.get(3).isInt32()) color = (ushort)args[3].toInt32();
  if (args.get(4).isInt32()) align = (Align)args[4].toInt32();
  if (args.get(5).isBoolean()) automap = args[5].toBoolean();
  if (JSVAL_IS_FUNCTION(cx, args.get(6))) click = args[6];
  if (JSVAL_IS_FUNCTION(cx, args.get(7))) hover = args[7];

  if (isValidPath(path))
    swprintf_s(path, _countof(path), L"%s", szText);
  else
    THROW_ERROR(cx, "Invalid image file path");

  JSObject* hook = BuildObject(cx, &image_class, image_methods, image_props);
  if (!hook) THROW_ERROR(cx, "Failed to create image object");

  ImageHook* pImageHook = new ImageHook(script, hook, path, x, y, color, automap, align, state);

  if (!pImageHook) THROW_ERROR(cx, "Failed to create ImageHook");

  JS_SetPrivate(hook, pImageHook);
  pImageHook->SetClickHandler(click);
  pImageHook->SetHoverHandler(hover);

  args.rval().setObjectOrNull(hook);

  return JS_TRUE;
}

JSAPI_PROP(image_x) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.setInt32(pImageHook->GetX());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_x_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  if (vp.isInt32()) pImageHook->SetX(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(image_y) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.setInt32(pImageHook->GetY());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_y_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  if (vp.isInt32()) pImageHook->SetY(vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(image_location) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.setString(JS_InternUCString(cx, pImageHook->GetImage()));
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_location_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  if (vp.isString()) {
    const wchar_t* pimage = JS_GetStringCharsZ(cx, vp.toString());
    if (!pimage) return JS_TRUE;
    pImageHook->SetImage(pimage);
  }
  return JS_TRUE;
}

JSAPI_PROP(image_visible) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.setBoolean(pImageHook->GetIsVisible());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_visible_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  if (vp.isBoolean()) pImageHook->SetIsVisible(!!vp.toBoolean());
  return JS_TRUE;
}

JSAPI_PROP(image_align) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.setInt32(pImageHook->GetAlign());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_align_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  if (vp.isInt32()) pImageHook->SetAlign((Align)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(image_zorder) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.setInt32(pImageHook->GetZOrder());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_zorder_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  if (vp.isInt32()) pImageHook->SetZOrder((ushort)vp.toInt32());
  return JS_TRUE;
}

JSAPI_PROP(image_click) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.set(pImageHook->GetClickHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_click_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  pImageHook->SetClickHandler(vp.get());
  return JS_TRUE;
}

JSAPI_PROP(image_hover) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  vp.set(pImageHook->GetHoverHandler());
  return JS_TRUE;
}

JSAPI_STRICT_PROP(image_hover_setter) {
  ImageHook* pImageHook = (ImageHook*)JS_GetPrivate(obj);
  if (!pImageHook) return JS_TRUE;

  pImageHook->SetHoverHandler(vp.get());
  return JS_TRUE;
}

JSAPI_FUNC(screenToAutomap) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() == 1) {
    // the arg must be an object with an x and a y that we can convert
    if (args[0].isObject()) {
      // get the params
      JSObject* arg = args[0].toObjectOrNull();
      JS::Value x, y;
      if (JS_GetProperty(cx, arg, "x", &x) == JS_FALSE || JS_GetProperty(cx, arg, "y", &y) == JS_FALSE)
        THROW_ERROR(cx, "Failed to get x and/or y values");
      if (!x.isInt32() || !y.isInt32()) THROW_ERROR(cx, "Input has an x or y, but they aren't the correct type!");
      int32_t ix, iy;
      if (JS_ValueToInt32(cx, x, &ix) == JS_FALSE || JS_ValueToInt32(cx, y, &iy))
        THROW_ERROR(cx, "Failed to convert x and/or y values");
      // convert the values
      POINT result = ScreenToAutomap(ix, iy);
      x = JS::Int32Value(result.x);
      y = JS::Int32Value(result.y);
      JSObject* res = JS_NewObject(cx, NULL, NULL, NULL);
      JS::Value* argv = JS_ARGV(cx, vp);
      if (JS_SetProperty(cx, res, "x", &argv[0]) == JS_FALSE || JS_SetProperty(cx, res, "y", &argv[1]) == JS_FALSE)
        THROW_ERROR(cx, "Failed to set x and/or y values");
      args.rval().setObjectOrNull(res);
    } else
      THROW_ERROR(cx, "Invalid object specified to screenToAutomap");
  } else if (args.length() == 2) {
    // the args must be ints
    if (args[0].isInt32() && args[1].isInt32()) {
      int32_t ix, iy;
      JS::Value* argv = args.array();
      if (JS_ValueToInt32(cx, argv[0], &ix) == JS_FALSE || JS_ValueToInt32(cx, argv[1], &iy) == JS_FALSE)
        THROW_ERROR(cx, "Failed to convert x and/or y values");
      // convert the values
      POINT result = ScreenToAutomap(ix, iy);
      argv[0] = JS::Int32Value(result.x);
      argv[1] = JS::Int32Value(result.y);
      JSObject* res = JS_NewObject(cx, NULL, NULL, NULL);
      if (JS_SetProperty(cx, res, "x", &argv[0]) == JS_FALSE || JS_SetProperty(cx, res, "y", &argv[1]) == JS_FALSE)
        THROW_ERROR(cx, "Failed to set x and/or y values");
      args.rval().setObjectOrNull(res);
    } else
      THROW_ERROR(cx, "screenToAutomap expects two arguments to be two integers");
  } else
    THROW_ERROR(cx, "Invalid arguments specified for screenToAutomap");
  return JS_TRUE;
}

JSAPI_FUNC(automapToScreen) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() == 1) {
    // the arg must be an object with an x and a y that we can convert
    if (args[0].isObject()) {
      // get the params
      JSObject* arg = args[0].toObjectOrNull();
      JS::Value x, y;
      if (JS_GetProperty(cx, arg, "x", &x) == JS_FALSE || JS_GetProperty(cx, arg, "y", &y) == JS_FALSE)
        THROW_ERROR(cx, "Failed to get x and/or y values");
      if (!x.isInt32() || !y.isInt32()) THROW_ERROR(cx, "Input has an x or y, but they aren't the correct type!");
      int32_t ix, iy;
      JSAutoRequest r(cx);
      if (JS_ValueToInt32(cx, x, &ix) == JS_FALSE || JS_ValueToInt32(cx, y, &iy)) {
        THROW_ERROR(cx, "Failed to convert x and/or y values");
      }
      // convert the values
      POINT result = {ix, iy};
      AutomapToScreen(&result);
      x = JS::Int32Value(ix);
      y = JS::Int32Value(iy);
      if (JS_SetProperty(cx, arg, "x", &x) == JS_FALSE || JS_SetProperty(cx, arg, "y", &y) == JS_FALSE) {
        THROW_ERROR(cx, "Failed to set x and/or y values");
      }
      args.rval().setObjectOrNull(arg);
    } else
      THROW_ERROR(cx, "Invalid object specified to automapToScreen");
  } else if (args.length() == 2) {
    // the args must be ints
    if (args[0].isInt32() && args[1].isInt32()) {
      int32_t ix, iy;
      JSAutoRequest r(cx);
      if (JS_ValueToInt32(cx, args[0], &ix) == JS_FALSE || JS_ValueToInt32(cx, args[1], &iy) == JS_FALSE) {
        THROW_ERROR(cx, "Failed to convert x and/or y values");
      }
      // convert the values
      POINT result = {ix, iy};
      AutomapToScreen(&result);
      args[0].setInt32(result.x);
      args[1].setInt32(result.y);
      JSObject* res = JS_NewObject(cx, NULL, NULL, NULL);
      if (JS_SetProperty(cx, res, "x", &args[0]) == JS_FALSE || JS_SetProperty(cx, res, "y", &args[1]) == JS_FALSE)
        THROW_ERROR(cx, "Failed to set x and/or y values");
      args.rval().setObjectOrNull(res);
    } else
      THROW_ERROR(cx, "automapToScreen expects two arguments to be two integers");
  } else
    THROW_ERROR(cx, "Invalid arguments specified for automapToScreen");
  return JS_TRUE;
}
