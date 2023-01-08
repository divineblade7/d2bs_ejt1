#include "d2bs/new_core/settings.h"

#include "d2bs/core/Control.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/new_util/localization.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

namespace d2bs {

Profile::Profile(const std::filesystem::path& filename, const std::wstring& section) {
  wchar_t defaultStarter[_MAX_FNAME] = L"", defaultConsole[_MAX_FNAME] = L"", defaultGame[_MAX_FNAME] = L"",
          scriptPath[_MAX_PATH] = L"";
  wchar_t difficulty[10], mode[256];
  auto path = filename.wstring();
  auto file = path.c_str();

  GetPrivateProfileStringW(section.c_str(), L"mode", L"single", mode, _countof(mode), file);
  GetPrivateProfileStringW(section.c_str(), L"character", L"ERROR", charname, _countof(charname), file);
  GetPrivateProfileStringW(section.c_str(), L"spdifficulty", L"0", difficulty, _countof(difficulty), file);
  GetPrivateProfileStringW(section.c_str(), L"username", L"ERROR", username, _countof(username), file);
  GetPrivateProfileStringW(section.c_str(), L"password", L"ERROR", password, _countof(password), file);
  GetPrivateProfileStringW(section.c_str(), L"gateway", L"ERROR", gateway, _countof(gateway), file);

  GetPrivateProfileStringW(section.c_str(), L"ScriptPath", L"scripts", scriptPath, _MAX_PATH, file);
  GetPrivateProfileStringW(section.c_str(), L"DefaultConsoleScript", L"", defaultConsole, _MAX_FNAME, file);
  GetPrivateProfileStringW(section.c_str(), L"DefaultGameScript", L"", defaultGame, _MAX_FNAME, file);
  GetPrivateProfileStringW(section.c_str(), L"DefaultStarterScript", L"", defaultStarter, _MAX_FNAME, file);

  wcscpy_s(szProfile, 256, section.c_str());
  script_dir = filename.parent_path() / scriptPath;

  if (wcslen(defaultConsole) > 0) {
    wcscpy_s(szConsole, _MAX_FNAME, defaultConsole);
  }
  if (wcslen(defaultGame) > 0) {
    wcscpy_s(szDefault, _MAX_FNAME, defaultGame);
  }
  if (wcslen(defaultStarter) > 0) {
    wcscpy_s(szStarter, _MAX_FNAME, defaultStarter);
  }

  int tmp = _wtoi(difficulty);

  if (tmp < 0 || tmp > 3) throw "Invalid difficulty.";

  diff = (char)tmp;

  type = PROFILETYPE_INVALID;

  switch (tolower(mode[0])) {
    case 's':
      type = PROFILETYPE_SINGLEPLAYER;
      break;
    case 'b':
      type = PROFILETYPE_BATTLENET;
      break;
    case 'o':
      type = PROFILETYPE_OPEN_BATTLENET;
      break;
    case 'h':
      type = PROFILETYPE_TCPIP_HOST;
      break;
    case 'j':
      type = PROFILETYPE_TCPIP_JOIN;
      break;
  }
}

static BOOL isOtherMP(ProfileType pt) {
  switch (pt) {
    case PROFILETYPE_OPEN_BATTLENET:
    case PROFILETYPE_TCPIP_HOST:
    case PROFILETYPE_TCPIP_JOIN:
      return true;
    default:
      return false;
  }
}

static BOOL isTcpIp(ProfileType pt) {
  switch (pt) {
    case PROFILETYPE_TCPIP_HOST:
    case PROFILETYPE_TCPIP_JOIN:
      return true;
    default:
      return false;
  }
}

DWORD Profile::login(const char** error) {
  // Sleep(10000);
  bool loginComplete = FALSE, skippedToBnet = TRUE;
  int location = 0;
  const char* errorMsg = nullptr;
  Control* pControl = NULL;
  unsigned int timeout = 0;

  /*
          clickedOnce is needed because, when in OOG_OTHER_MULTIPLAYER
          the clickControl () is done twice and the second time it is
          failing because the button is not there anymore.
  */
  int clickedOnce = false;

  Vars.bBlockKeys = Vars.bBlockMouse = TRUE;

  while (!loginComplete) {
    location = OOG_GetLocation();
    switch (location) {
      case OOG_D2SPLASH:
        clickControl(*p_D2WIN_FirstControl);
        break;

      case OOG_CHAR_SELECT:
        // Sleep(5000);
        if (!OOG_SelectCharacter(charname)) errorMsg = "Invalid character name";
        break;
      case OOG_MAIN_MENU:
        if (type == PROFILETYPE_SINGLEPLAYER)
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 264, 324, 272, 35)))
            errorMsg = "Failed to click the Single button?";
        if (type == PROFILETYPE_BATTLENET) {
          OOG_SelectGateway(gateway, 256);
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 264, 366, 272, 35)))
            errorMsg = "Failed to click the 'Battle.net' button?";
        }
        if (isOtherMP(type)) {
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 264, 433, 272, 35)))
            errorMsg = "Failed to click the 'Other Multiplayer' button?";
          else
            skippedToBnet = FALSE;
        }
        break;
      case OOG_LOGIN:
        if ((type == PROFILETYPE_SINGLEPLAYER || isOtherMP(type)) && skippedToBnet) {
          if (!clickControl(findControl(6, L"EXIT", -1, 33, 572, 128, 35)))
            errorMsg = "Failed to click the exit button?";
          break;
        }
        pControl = findControl(1, (const wchar_t*)NULL, -1, 322, 342, 162, 19);
        if (pControl) {
          SetControlText(pControl, username);
        } else
          errorMsg = "Failed to set the 'Username' text-edit box.";
        // Password text-edit box
        pControl = findControl(1, (const wchar_t*)NULL, -1, 322, 396, 162, 19);
        if (pControl) {
          SetControlText(pControl, password);
        } else
          errorMsg = "Failed to set the 'Password' text-edit box.";

        pControl = findControl(6, (const wchar_t*)NULL, -1, 264, 484, 272, 35);
        if (pControl)
          if (!clickControl(pControl)) errorMsg = "Failed to click the 'Log in' button?";
        timeout++;
        break;
      case OOG_DIFFICULTY: {
        Control *normal = findControl(CONTROL_BUTTON, (const wchar_t*)NULL, -1, 264, 297, 272, 35),
                *nightmare = findControl(CONTROL_BUTTON, (const wchar_t*)NULL, -1, 264, 340, 272, 35),
                *hell = findControl(CONTROL_BUTTON, (const wchar_t*)NULL, -1, 264, 383, 272, 35);

        switch (diff) {
          case 0:  // normal button
            if (normal->dwDisabled != 0x0d || !clickControl(normal))
              errorMsg = "Failed to click the 'Normal Difficulty' button?";
            break;
          case 1:  // nightmare button
            if (nightmare->dwDisabled != 0x0d || !clickControl(nightmare))
              errorMsg = "Failed to click the 'Nightmare Difficulty' button?";
            break;
          case 2:  // hell button
            if (hell->dwDisabled != 0x0d || !clickControl(hell))
              errorMsg = "Failed to click the 'Hell Difficulty' button?";
            break;
          case 3:  // hardest difficulty available
            if ((hell->dwDisabled == 0x0d && clickControl(hell)) ||
                (nightmare->dwDisabled == 0x0d && clickControl(nightmare)) ||
                (normal->dwDisabled == 0x0d && clickControl(normal))) {
              break;
            }
            errorMsg = "Failed to click ANY difficulty button?";
            break;
          default:
            errorMsg = "Invalid single player difficulty level specified!";
            break;
        }
      }
      case OOG_OTHER_MULTIPLAYER:
        // Open Battle.net
        if (type == PROFILETYPE_OPEN_BATTLENET)
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 264, 310, 272, 35)))
            errorMsg = "Failed to click the 'Open Battle.net' button?";
        // TCP/IP Game
        if (isTcpIp(type))
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 264, 350, 272, 35)) && !clickedOnce)
            errorMsg = "Failed to click the 'TCP/IP Game' button?";
          else
            clickedOnce = true;

        break;
      case OOG_TCP_IP:
        if (type == PROFILETYPE_TCPIP_HOST)
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 265, 206, 272, 35)))
            errorMsg = "Failed to click the 'Host Game' button?";
        if (type == PROFILETYPE_TCPIP_JOIN)
          if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 265, 264, 272, 35)))
            errorMsg = "Failed to click the 'Join Game' button?";
        break;
      case OOG_ENTER_IP_ADDRESS:
        if (_wcsicmp(ip, L"")) {
          pControl = findControl(1, (const wchar_t*)NULL, -1, 300, 268, -1, -1);
          if (pControl) {
            SetControlText(pControl, ip);

            // Click the OK button
            // Sleep(5000);
            if (!clickControl(findControl(6, (const wchar_t*)NULL, -1, 421, 337, 96, 32))) {
              errorMsg = "Failed to click the OK button";
            }
          } else
            errorMsg = "Failed to find the 'Host IP Address' text-edit box.";
        } else
          errorMsg = "Could not get the IP address from the profile in the d2bs.ini file.";

        break;
      case OOG_UNABLE_TO_CONNECT_TCPIP:
        errorMsg = "Failed to join Host IP Address";
        break;
      case OOG_MAIN_MENU_CONNECTING:
      case OOG_CHARACTER_SELECT_PLEASE_WAIT:
      case OOG_PLEASE_WAIT:
      case OOG_GATEWAY:
      case OOG_CHARACTER_SELECT_NO_CHARS:
      case OOG_CONNECTING:
      case OOG_NONE:
        timeout++;
        break;
      case OOG_LOBBY:
      case OOG_INLINE:
      case OOG_CHAT:
      case OOG_CREATE:
      case OOG_JOIN:
      case OOG_LADDER:
      case OOG_CHANNEL:
      case OOG_GAME_EXIST:
      case OOG_GAME_DOES_NOT_EXIST:
        loginComplete = TRUE;
        break;
      case OOG_UNABLE_TO_CONNECT:
        errorMsg = "Unable to connect";
        break;
      case OOG_CDKEY_IN_USE:
        errorMsg = "CD-Key in use";
        break;
      case OOG_LOGIN_ERROR:
        errorMsg = "Bad account or password";
        break;
      case OOG_REALM_DOWN:
        errorMsg = "Realm Down";
        break;
      default:
        errorMsg = "Unhandled login location";
        break;
    }

    if (_strcmpi(errorMsg, "")) {
      Vars.bBlockKeys = Vars.bBlockMouse = FALSE;
      *error = errorMsg;
      return 2;
    }

    if ((timeout * 100) > Vars.settings.maxLoginTime) {
      Vars.bBlockKeys = Vars.bBlockMouse = FALSE;
      *error = "login time out";
      return 1;
    }

    if (ClientState() == ClientStateInGame) loginComplete = TRUE;

    Sleep(100);
  }

  Vars.bBlockKeys = Vars.bBlockMouse = FALSE;

  return 0;
}

bool Settings::load(const std::filesystem::path& filename) {
  wchar_t scriptPath[_MAX_PATH], hosts[256], debug[6], quitOnHostile[6], quitOnError[6], startAtMenu[6],
      disableCache[6], memUsage[6], gamePrint[6], useProfilePath[6], logConsole[6], enableUnsupported[6],
      forwardMessageBox[6], consoleFont[6];
  int maxGameTime = 0;
  int gameTimeout = 0;

  auto path = filename.wstring();
  auto fname = path.c_str();

  GetPrivateProfileStringW(L"settings", L"ScriptPath", L"scripts", scriptPath, _MAX_PATH, fname);
  GetPrivateProfileStringW(L"settings", L"DefaultConsoleScript", L"", szConsole, _MAX_FNAME, fname);
  GetPrivateProfileStringW(L"settings", L"DefaultGameScript", L"default.dbj", szDefault, _MAX_FNAME, fname);
  GetPrivateProfileStringW(L"settings", L"DefaultStarterScript", L"starter.dbj", szStarter, _MAX_FNAME, fname);
  GetPrivateProfileStringW(L"settings", L"Hosts", L"", hosts, 256, fname);
  maxGameTime = GetPrivateProfileIntW(L"settings", L"MaxGameTime", 0, fname);
  GetPrivateProfileStringW(L"settings", L"Debug", L"false", debug, 6, fname);
  GetPrivateProfileStringW(L"settings", L"QuitOnHostile", L"false", quitOnHostile, 6, fname);
  GetPrivateProfileStringW(L"settings", L"QuitOnError", L"false", quitOnError, 6, fname);
  GetPrivateProfileStringW(L"settings", L"StartAtMenu", L"true", startAtMenu, 6, fname);
  GetPrivateProfileStringW(L"settings", L"DisableCache", L"true", disableCache, 6, fname);
  GetPrivateProfileStringW(L"settings", L"MemoryLimit", L"100", memUsage, 6, fname);
  GetPrivateProfileStringW(L"settings", L"UseGamePrint", L"false", gamePrint, 6, fname);
  gameTimeout = GetPrivateProfileIntW(L"settings", L"GameReadyTimeout", 5, fname);
  GetPrivateProfileStringW(L"settings", L"UseProfileScript", L"false", useProfilePath, 6, fname);
  GetPrivateProfileStringW(L"settings", L"LogConsoleOutput", L"false", logConsole, 6, fname);
  GetPrivateProfileStringW(L"settings", L"EnableUnsupported", L"false", enableUnsupported, 6, fname);
  GetPrivateProfileStringW(L"settings", L"ForwardMessageBox", L"false", forwardMessageBox, 6, fname);
  GetPrivateProfileStringW(L"settings", L"ConsoleFont", L"0", consoleFont, 6, fname);

  wchar_t _maxLoginTime[10], _maxCharTime[10];
  GetPrivateProfileStringW(L"settings", L"MaxLoginTime", L"5", _maxLoginTime, _countof(_maxLoginTime), fname);
  GetPrivateProfileStringW(L"settings", L"MaxCharSelectTime", L"5", _maxCharTime, _countof(_maxCharTime), fname);

  maxLoginTime = abs(_wtoi(_maxLoginTime) * 1000);
  maxCharTime = abs(_wtoi(_maxCharTime) * 1000);

  script_dir = filename.parent_path() / scriptPath;

  auto hostsutf8 = util::wide_to_utf8(hosts);
  strcpy_s(szHosts, 256, hostsutf8.c_str());

  dwGameTime = GetTickCount();
  dwMaxGameTime = abs(maxGameTime * 1000);
  dwGameTimeout = abs(gameTimeout * 1000);

  bQuitOnHostile = util::to_bool(quitOnHostile);
  bQuitOnError = util::to_bool(quitOnError);
  bStartAtMenu = util::to_bool(startAtMenu);
  bDisableCache = util::to_bool(disableCache);
  bUseGamePrint = util::to_bool(gamePrint);
  bUseProfileScript = util::to_bool(useProfilePath);
  bLogConsole = util::to_bool(logConsole);
  bEnableUnsupported = util::to_bool(enableUnsupported);
  bForwardMessageBox = util::to_bool(forwardMessageBox);
  dwMemUsage = abs(_wtoi(memUsage));
  dwConsoleFont = abs(_wtoi(consoleFont));
  if (dwMemUsage < 1) {
    dwMemUsage = 500;
  }
  dwMemUsage *= 1024 * 1024;

  wchar_t sections[65535]{};
  uint32_t count = GetPrivateProfileStringW(nullptr, nullptr, nullptr, sections, 65535, fname);
  for (uint32_t i = 0; i < count;) {
    std::wstring name(sections + i);
    i += name.length() + 1;
    if (name != L"settings") {
      profiles_[name] = Profile{filename, name};
    }
  }

  return true;
}

bool Settings::has_profile(const std::wstring& name) {
  return profiles_.contains(name);
}

bool Settings::set_profile(const std::wstring& name) {
  if (bUseProfileScript != TRUE || !has_profile(name)) {
    return false;
  }

  Profile& profile = profiles_[name];
  script_dir = profile.script_dir;
  wcscpy_s(szProfile, _MAX_FNAME, profile.szProfile);
  wcscpy_s(szStarter, _MAX_FNAME, profile.szStarter);
  wcscpy_s(szConsole, _MAX_FNAME, profile.szConsole);
  wcscpy_s(szDefault, _MAX_FNAME, profile.szDefault);
  bUseProfileScript = FALSE;
  return true;
}

Profile* Settings::get_profile(const std::wstring& name) {
  if (has_profile(name)) {
    return &profiles_[name];
  }
  return nullptr;
}

Profile* Settings::add_profile(ProfileType pt, const wchar_t* _ipUsername, const wchar_t* _password,
                               const wchar_t* _charname, const wchar_t* _gateway, const wchar_t _diff,
                               unsigned int _maxLoginTime, unsigned int _maxCharTime) {
  Profile profile;
  profile.type = pt;
  wcscpy_s(profile.username, wcslen(profile.username), _ipUsername);
  wcscpy_s(profile.password, wcslen(profile.password), _password);
  wcscpy_s(profile.gateway, wcslen(profile.gateway), _gateway);
  wcscpy_s(profile.charname, wcslen(profile.charname), _charname);
  profile.diff = _diff;
  maxLoginTime = _maxLoginTime;
  maxCharTime = _maxCharTime;

  profiles_[_charname] = profile;
  return &profiles_[_charname];
}

}  // namespace d2bs
