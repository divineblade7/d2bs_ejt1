#pragma once

#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/script/Script.h"
#include "d2bs/script/ScriptEngine.h"

#include <algorithm>
#include <list>
#include <windows.h>

void DrawLogo(void);

typedef unsigned short ushort;

class Genhook;

typedef std::list<Genhook*> HookList;
typedef HookList::iterator HookIterator;

struct HookClickHelper {
  int button;
  POINT point;
};

typedef bool (*HookCallback)(Genhook*, void*, uint);

enum Align { Left, Right, Center };
enum ScreenhookState { OOG, IG, Perm };

bool DrawHook(Genhook* hook, void* argv, uint argc);
bool CleanHook(Genhook* hook, void* argv, uint argc);
bool ClickHook(Genhook* hook, void* argv, uint argc);
bool HoverHook(Genhook* hook, void* argv, uint argc);

class Genhook {
 private:
  static bool init;
  static HookList visible, invisible;
  static CRITICAL_SECTION globalSection;

 protected:
  Script* owner;
  ScreenhookState gameState;
  Align alignment;
  jsval clicked, hovered;
  JSObject* self;
  bool isAutomap, isVisible;
  ushort opacity, zorder;
  POINT location;
  // CRITICAL_SECTION hookSection;

 public:
  Genhook(Script* nowner, JSObject* nself, uint x, uint y, ushort nopacity, bool nisAutomap = false,
          Align nalign = Left, ScreenhookState ngameState = Perm);
  virtual ~Genhook(void);

  Genhook(const Genhook&) = delete;
  Genhook& operator=(const Genhook&) = delete;

  static void DrawAll(ScreenhookState type);

  static bool ForEachHook(HookCallback proc, void* argv, uint argc);
  static bool ForEachVisibleHook(HookCallback proc, void* argv, uint argc);
  static bool ForEachInvisibleHook(HookCallback proc, void* argv, uint argc);
  static bool ForEachVisibleHookUnLocked(HookCallback proc, void* argv, uint argc);
  static void Clean(Script* owner);
  static void Initialize(void) {
    InitializeCriticalSection(&globalSection);
    init = true;
  }
  static void Destroy(void) {
    init = false;
    DeleteCriticalSection(&globalSection);
  }
  virtual void Draw(void) = 0;

 public:
  Script* getOwner() {
    return owner;
  }
  bool Click(int button, POINT* loc);
  void Hover(POINT* loc);

  bool IsInRange(POINT* pt) {
    return IsInRange(pt->x, pt->y);
  }
  virtual bool IsInRange(int dx, int dy) = 0;

  void Move(POINT* dist) {
    Move(dist->x, dist->y);
  }
  void Move(uint nx, uint ny) {
    SetX(GetX() + nx);
    SetY(GetY() + ny);
  }

  void SetX(uint x) {
    Lock();
    location.x = x;
    Unlock();
  }
  void SetY(uint y) {
    Lock();
    location.y = y;
    Unlock();
  }
  void SetAlign(Align nalign) {
    Lock();
    alignment = nalign;
    Unlock();
  }
  void SetOpacity(ushort nopacity) {
    Lock();
    opacity = nopacity;
    Unlock();
  }
  void SetIsVisible(bool nisVisible) {
    Lock();
    EnterCriticalSection(&globalSection);
    if (!nisVisible) {
      if (isVisible) {
        visible.remove(this);
        invisible.push_back(this);
      }
    } else {
      if (!isVisible) {
        invisible.remove(this);
        visible.push_back(this);
      }
    }
    isVisible = nisVisible;
    Unlock();
    LeaveCriticalSection(&globalSection);
  }
  void SetZOrder(ushort norder) {
    Lock();
    zorder = norder;
    Unlock();
  }
  void SetClickHandler(jsval handler);
  void SetHoverHandler(jsval handler);

  POINT GetLocation(void) const {
    return location;
  }
  uint GetX(void) const {
    return location.x;
  }
  uint GetY(void) const {
    return location.y;
  }
  Align GetAlign(void) const {
    return alignment;
  }
  ushort GetOpacity(void) const {
    return opacity;
  }
  ScreenhookState GetGameState(void) const {
    return gameState;
  }
  bool GetIsAutomap(void) const {
    return isAutomap;
  }
  bool GetIsVisible(void) const {
    return isVisible;
  }
  ushort GetZOrder(void) const {
    return zorder;
  }
  jsval GetClickHandler(void) {
    return clicked;
  }
  jsval GetHoverHandler(void) {
    return hovered;
  }

  static void EnterGlobalSection() {
    EnterCriticalSection(&globalSection);
  }
  static void LeaveGlobalSection() {
    LeaveCriticalSection(&globalSection);
  }

  void Lock() { /* EnterCriticalSection(&hookSection);*/
  }
  void Unlock() { /* LeaveCriticalSection(&hookSection);*/
  }
};

class TextHook : public Genhook {
 private:
  wchar_t* text;
  ushort font, color;

 public:
  TextHook(Script* owner, JSObject* nself, const wchar_t* text, uint x, uint y, ushort nfont, ushort ncolor,
           bool automap = false, Align align = Left, ScreenhookState state = Perm)
      : Genhook(owner, nself, x, y, 0, automap, align, state), text(NULL), font(nfont), color(ncolor) {
    this->text = _wcsdup(text);
  }
  ~TextHook(void) {
    free(text);
  }

  TextHook(const TextHook&) = delete;
  TextHook& operator=(const TextHook&) = delete;

 protected:
  void Draw(void);

 public:
  bool IsInRange(int dx, int dy);

  void SetFont(ushort nfont) {
    Lock();
    font = nfont;
    Unlock();
  }
  void SetColor(ushort ncolor) {
    Lock();
    color = ncolor;
    Unlock();
  }
  void SetText(const wchar_t* ntext);

  ushort GetFont(void) const {
    return font;
  }
  ushort GetColor(void) const {
    return color;
  }
  const wchar_t* GetText(void) const {
    return text;
  }
};

class ImageHook : public Genhook {
 private:
  wchar_t* location;
  CellFile* image;
  ushort color;

 public:
  ImageHook(Script* owner, JSObject* nself, const wchar_t* nloc, uint x, uint y, ushort ncolor, bool automap = false,
            Align align = Left, ScreenhookState state = Perm)
      : Genhook(owner, nself, x, y, 0, automap, align, state), color(ncolor), image(NULL), location(NULL) {
    location = _wcsdup(nloc);
    image = LoadCellFile(location, 3);
  }
  ~ImageHook(void) {
    free(location);
    delete[] image;
  }

  ImageHook(const ImageHook&) = delete;
  ImageHook& operator=(const ImageHook&) = delete;

 protected:
  void Draw(void);

 public:
  bool IsInRange(int dx, int dy);

  void SetImage(const wchar_t* nimage);
  void SetColor(ushort ncolor) {
    Lock();
    color = ncolor;
    Unlock();
  }

  const wchar_t* GetImage(void) const {
    return location;
  }
  ushort GetColor(void) const {
    return color;
  }
};

class LineHook : public Genhook {
 private:
  uint x2, y2;
  ushort color;

 public:
  LineHook(Script* owner, JSObject* nself, uint x, uint y, uint nx2, uint ny2, ushort ncolor, bool automap = false,
           Align align = Left, ScreenhookState state = Perm)
      : Genhook(owner, nself, x, y, 0, automap, align, state), x2(nx2), y2(ny2), color(ncolor) {}
  ~LineHook(void) {}

  LineHook(const LineHook&) = delete;
  LineHook& operator=(const LineHook&) = delete;

 protected:
  void Draw(void);

 public:
  bool IsInRange(int, int) {
    return false;
  }

  void SetX2(uint nx2) {
    Lock();
    x2 = nx2;
    Unlock();
  }
  void SetY2(uint ny2) {
    Lock();
    y2 = ny2;
    Unlock();
  }
  void SetColor(ushort ncolor) {
    Lock();
    color = ncolor;
    Unlock();
  }

  uint GetX2(void) const {
    return x2;
  }
  uint GetY2(void) const {
    return y2;
  }
  ushort GetColor(void) const {
    return color;
  }
};

class BoxHook : public Genhook {
 private:
  uint xsize, ysize;
  ushort color;

 public:
  BoxHook(Script* owner, JSObject* nself, uint x, uint y, uint nxsize, uint nysize, ushort ncolor, ushort opacity,
          bool automap = false, Align align = Left, ScreenhookState state = Perm)
      : Genhook(owner, nself, x, y, opacity, automap, align, state), xsize(nxsize), ysize(nysize), color(ncolor) {}
  ~BoxHook(void) {}

  BoxHook(const BoxHook&) = delete;
  BoxHook& operator=(const BoxHook&) = delete;

 protected:
  void Draw(void);

 public:
  bool IsInRange(int dx, int dy);

  void SetXSize(uint nxsize) {
    Lock();
    xsize = nxsize;
    Unlock();
  }
  void SetYSize(uint nysize) {
    Lock();
    ysize = nysize;
    Unlock();
  }
  void SetColor(ushort ncolor) {
    Lock();
    color = ncolor;
    Unlock();
  }

  uint GetXSize(void) const {
    return xsize;
  }
  uint GetYSize(void) const {
    return ysize;
  }
  ushort GetColor(void) const {
    return color;
  }
};

class FrameHook : public Genhook {
 private:
  uint xsize, ysize;

 public:
  FrameHook(Script* owner, JSObject* nself, uint x, uint y, uint nxsize, uint nysize, bool automap = false,
            Align align = Left, ScreenhookState state = Perm)
      : Genhook(owner, nself, x, y, 0, automap, align, state), xsize(nxsize), ysize(nysize) {}
  ~FrameHook(void) {}

  FrameHook(const FrameHook&) = delete;
  FrameHook& operator=(const FrameHook&) = delete;

 protected:
  void Draw(void);

 public:
  bool IsInRange(int dx, int dy);

  void SetXSize(uint nxsize) {
    Lock();
    xsize = nxsize;
    Unlock();
  }
  void SetYSize(uint nysize) {
    Lock();
    ysize = nysize;
    Unlock();
  }

  uint GetXSize(void) const {
    return xsize;
  }
  uint GetYSize(void) const {
    return ysize;
  }
};
