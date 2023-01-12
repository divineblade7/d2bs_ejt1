#include "d2bs/script/api/JSMenu.h"

#include "d2bs/diablo/Constants.h"
#include "d2bs/diablo/D2Helpers.h"
#include "d2bs/script/api/JSControl.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

JSAPI_FUNC(my_login) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setUndefined();
  if (ClientState() != ClientStateMenu) return JS_TRUE;

  const wchar_t* name = NULL;
  const char* error;

  if (!args[0].isString()) {
    if (Vars.settings.szProfile != NULL) {
      name = Vars.settings.szProfile;
    } else
      THROW_ERROR(cx, "Invalid profile specified!");
  } else {
    name = JS_GetStringCharsZ(cx, args[0].toString());
    wcscpy_s(Vars.settings.szProfile, 256, name);
  }

  if (!name) THROW_ERROR(cx, "Could not get profile!");

  d2bs::Profile* profile = Vars.settings.get_profile(name);
  if (!profile) THROW_ERROR(cx, "Profile does not exist!");

  profile->login(&error);
  return JS_TRUE;
}

JSAPI_FUNC(my_selectChar) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1 || !args[0].isString())
    THROW_ERROR(cx, "Invalid parameters specified to selectCharacter");

  const wchar_t* name = JS_GetStringCharsZ(cx, JS_ValueToString(cx, args[0]));

  d2bs::Profile* profile = Vars.settings.get_profile(name);
  if (!profile) THROW_ERROR(cx, "Invalid profile specified");

  args.rval().setBoolean(OOG_SelectCharacter(profile->charname));

  return JS_TRUE;
}

JSAPI_FUNC(my_createGame) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  const wchar_t *name = NULL, *pass = NULL;
  jschar *jsname = NULL, *jspass = NULL;
  int32_t diff = 3;
  if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "W/Wi", &jsname, &jspass, &diff)) {
    THROW_ERROR(cx, "Invalid arguments specified to createGame");
  }
  name = JS_GetStringCharsZ(cx, args[0].toString());
  pass = JS_GetStringCharsZ(cx, args[1].toString());

  if (!pass) pass = L"";

  if (wcslen(name) > 15 || wcslen(pass) > 15) THROW_ERROR(cx, "Invalid game name or password length");

  if (!OOG_CreateGame(name, pass, diff)) THROW_ERROR(cx, "createGame failed");

  return JS_TRUE;
}

JSAPI_FUNC(my_joinGame) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  jschar *jsname = NULL, *jspass = NULL;
  const wchar_t *name = NULL, *pass = NULL;
  if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "W/W", &jsname, &jspass)) {
    THROW_ERROR(cx, "Invalid arguments specified to createGame");
  }
  name = JS_GetStringCharsZ(cx, args[0].toString());
  pass = JS_GetStringCharsZ(cx, args[1].toString());
  if (!pass) pass = L"";

  if (wcslen(name) > 15 || wcslen(pass) > 15) THROW_ERROR(cx, "Invalid game name or password length");

  if (!OOG_JoinGame(name, pass)) THROW_ERROR(cx, "joinGame failed");

  return JS_TRUE;
}

JSAPI_FUNC(my_addProfile) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  // validate the args...
  const wchar_t *profile, *mode, *gateway, *username, *password, *charname;
  profile = mode = gateway = username = password = charname = NULL;
  int spdifficulty = 3;
  if (args.length() < 6 || args.length() > 7) THROW_ERROR(cx, "Invalid arguments passed to addProfile");

  const wchar_t** argv[] = {&profile, &mode, &gateway, &username, &password, &charname};
  for (uint32_t i = 0; i < 6; i++) {
    if (!args[i].isString()) {
      THROW_ERROR(cx, "Invalid argument passed to addProfile");
    } else {
      *argv[i] = JS_GetStringCharsZ(cx, args[i].toString());
    }
  }

  for (int i = 0; i < 6; i++) {
    if (!(*argv[i])) THROW_ERROR(cx, "Failed to convert string");
  }

  if (args.length() == 7) spdifficulty = args[6].toInt32();

  if (spdifficulty > 3 || spdifficulty < 0) THROW_ERROR(cx, "Invalid argument passed to addProfile");

  auto path = (Vars.working_dir / "d2bs.ini").wstring();
  auto file = path.c_str();
  if (!Vars.settings.has_profile(*argv[0])) {
    wchar_t settings[600] = L"";
    swprintf_s(settings, _countof(settings),
               L"mode=%s\tgateway=%s\tusername=%s\tpassword=%s\tcharacter=%s\tspdifficulty=%d\t", mode, gateway,
               username, password, charname, spdifficulty);

    StringReplace(settings, '\t', '\0', 600);
    WritePrivateProfileSectionW(*argv[0], settings, file);
  }

  return JS_TRUE;
}

JSAPI_FUNC(my_getOOGLocation) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  args.rval().setInt32(OOG_GetLocation());
  return JS_TRUE;
}

JSAPI_FUNC(my_createCharacter) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  if (ClientState() != ClientStateMenu) return JS_TRUE;

  const wchar_t* name = NULL;
  jschar* jsname = NULL;
  int32_t type = -1;
  JSBool hc = JS_FALSE, ladder = JS_FALSE;
  JSAutoRequest r(cx);
  if (!JS_ConvertArguments(cx, argc, args.array(), "Wi/bb", &jsname, &type, &hc, &ladder)) {
    THROW_ERROR(cx, "Failed to Convert Args createCharacter");
  }
  name = JS_GetStringCharsZ(cx, args[0].toString());

  args.rval().setBoolean(OOG_CreateCharacter(name, type, !!hc, !!ladder));
  return JS_TRUE;
}
