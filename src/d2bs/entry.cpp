#include "d2bs/new_core/application.h"
#include "d2bs/variables.h"

#include <Windows.h>

DWORD WINAPI D2BS_MainThread(void* userdata) {
  HMODULE handle = static_cast<HMODULE>(userdata);
  Vars.hModule = handle;

  // grab root directory from input module pointer
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(handle, path, MAX_PATH);
  std::filesystem::path root_dir_ = path;
  // remove filename from path and make preferred, '\\' instead of '/'
  root_dir_.remove_filename();

  Vars.working_dir = root_dir_;       // DEPRECATED
  Vars.log_dir = root_dir_ / "logs";  // DEPRECATED

  // create log directory if it does not exist
  if (!std::filesystem::exists(Vars.log_dir)) {
    std::filesystem::create_directory(Vars.log_dir);
  }

  try {
    auto app = std::make_unique<d2bs::Application>();
    app->run();
    app.reset();
  } catch (const std::exception& e) {
    // log error
  }

  FreeLibraryAndExitThread(Vars.hModule, 0);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hModule);

      CreateThread(nullptr, 0, D2BS_MainThread, hModule, 0, nullptr);
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
