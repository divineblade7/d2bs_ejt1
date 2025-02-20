#include "d2bs/engine.h"

#include <Windows.h>

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hModule);

      sEngine->startup(hModule);
      break;
    case DLL_PROCESS_DETACH:
      sEngine->shutdown();
      break;
  }
  return TRUE;
}
