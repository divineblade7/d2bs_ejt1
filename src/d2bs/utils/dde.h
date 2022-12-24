#pragma once

// clang-format off
#include <windows.h>
#include <ddeml.h>
// clang-format on

class DdeServer {
 public:
  DdeServer() noexcept = default;
  ~DdeServer() noexcept = default;

  bool init();
  bool shutdown();

  bool send(int mode, const char* pszDDEServer, const char* pszTopic, const char* pszItem, const char* pszData,
            char** result, unsigned int size);

 private:
  DWORD srv_inst_ = 0;
  HSZ hszD2BSns;
};
