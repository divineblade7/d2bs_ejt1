#pragma once

#include "d2bs/script/js32.h"

class AutoRoot {
 public:
  AutoRoot() : var(JSVAL_NULL), count(0) {}
  AutoRoot(JSContext* cx, jsval var);
  AutoRoot(jsval var);
  ~AutoRoot();

  AutoRoot(const AutoRoot&) = delete;
  AutoRoot& operator=(const AutoRoot&) = delete;

  jsval* value() {
    return &var;
  }

  jsval operator*() {
    return *value();
  }

  bool operator==(AutoRoot& other) {
    return value() == other.value();
  }

  void Take();
  void Release();

 private:
  jsval var;
  int count;
  JSContext* cx;
};
