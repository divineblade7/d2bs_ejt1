#pragma once

#include "d2bs/script/js32.h"

class AutoRoot {
 public:
  AutoRoot() {}
  AutoRoot(JSContext* cx, JS::Value var);
  ~AutoRoot();

  AutoRoot(const AutoRoot&) = delete;
  AutoRoot& operator=(const AutoRoot&) = delete;

  JS::Value* value() {
    return &var;
  }

  JS::Value operator*() {
    return *value();
  }

  bool operator==(AutoRoot& other) {
    return value() == other.value();
  }

  void Take();
  void Release();

 private:
  JSContext* cx = nullptr;
  JS::Value var = JS::NullValue();
  int count = 0;
};
