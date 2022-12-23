#include "d2bs/D2BS.h"

#include <Windows.h>

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      sEngine->Startup(hModule, lpParameter);
      break;
    case DLL_PROCESS_DETACH:
      sEngine->Shutdown();
      break;
  }
  return TRUE;
}
