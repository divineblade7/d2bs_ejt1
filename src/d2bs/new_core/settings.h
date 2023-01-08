#pragma once

#include <Windows.h>
#include <filesystem>
#include <unordered_map>

namespace d2bs {

enum ProfileType {
  PROFILETYPE_INVALID,
  PROFILETYPE_SINGLEPLAYER,
  PROFILETYPE_BATTLENET,
  PROFILETYPE_OPEN_BATTLENET,
  PROFILETYPE_TCPIP_HOST,
  PROFILETYPE_TCPIP_JOIN
};

class Profile {
 public:
  Profile() = default;
  Profile(const std::filesystem::path& filename, const std::wstring& section);

  ProfileType type;
  union {
    wchar_t ip[16];
    wchar_t username[48];
  };
  wchar_t password[256];
  wchar_t gateway[256];
  wchar_t charname[24];
  wchar_t diff;

  std::filesystem::path script_dir;
  wchar_t szProfile[256];
  wchar_t szStarter[_MAX_FNAME];
  wchar_t szConsole[_MAX_FNAME];
  wchar_t szDefault[_MAX_FNAME];

  DWORD login(const char** error);
};

class Settings {
 public:
  bool load(const std::filesystem::path& filename);

  bool has_profile(const std::wstring& name);
  bool set_profile(const std::wstring& name);
  Profile* get_profile(const std::wstring& name);
  Profile* add_profile(ProfileType pt, const wchar_t* _ipUsername, const wchar_t* _password, const wchar_t* _charname,
                       const wchar_t* _gateway, const wchar_t _diff = 0, unsigned int _maxLoginTime = 5000,
                       unsigned int _maxCharTime = 5000);

  unsigned int maxLoginTime;
  unsigned int maxCharTime;

  wchar_t szProfile[256];
  wchar_t szStarter[_MAX_FNAME];
  wchar_t szConsole[_MAX_FNAME];
  wchar_t szDefault[_MAX_FNAME];

  std::filesystem::path script_dir;
  char szHosts[256];
  DWORD dwGameTime;
  DWORD dwMaxGameTime;
  DWORD dwGameTimeout;
  BOOL bQuitOnError;
  BOOL bQuitOnHostile;
  BOOL bStartAtMenu;
  BOOL bDisableCache;
  BOOL bUseGamePrint;
  BOOL bUseProfileScript;
  BOOL bLogConsole;
  BOOL bEnableUnsupported;
  BOOL bForwardMessageBox;
  int dwMemUsage;
  int dwConsoleFont;

 private:
  std::unordered_map<std::wstring, Profile> profiles_;
};

}  // namespace d2bs
