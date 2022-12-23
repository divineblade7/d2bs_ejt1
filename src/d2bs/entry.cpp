#include "d2bs/D2BS.h"

#include <Windows.h>

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hModule);

      sEngine->startup(hModule, lpParameter);
      break;
    case DLL_PROCESS_DETACH:
      sEngine->shutdown();
      break;
  }
  return TRUE;
}
