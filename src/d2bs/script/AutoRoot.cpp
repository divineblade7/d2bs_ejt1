#include "d2bs/script/AutoRoot.h"

#include <intrin.h> // __debugbreak

AutoRoot::AutoRoot(JSContext* ncx, jsval nvar) : cx(ncx), var(nvar), count(0) {
  Take();
}

AutoRoot::~AutoRoot() {
  if (count < 0) {
    fprintf(stderr, "AutoRoot failed: Count is still %i, but the root is being destroyed", count);
    __debugbreak();
    exit(3);
  }
  JS_BeginRequest(cx);
  JS_RemoveValueRoot(cx, &var);
  JS_EndRequest(cx);
}

void AutoRoot::Take() {
  count++;
  JS_AddNamedValueRoot(cx, &var, "AutoRoot");
}

void AutoRoot::Release() {
  count--;
  if (count < 0) {
    fprintf(stderr, "Improper AutoRoot usage: Count is less than 0");
    __debugbreak();
    exit(3);
  }
}
