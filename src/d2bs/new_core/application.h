#pragma once

#include "d2bs/script/ScriptEngine.h"

namespace d2bs {

class Application {
 public:
  Application();
  ~Application();

  void run();

 private:
  void parse_commandline_args();
  void init_settings();
  bool init_hooks();

  void handle_enter_game();
};

}  // namespace d2bs
