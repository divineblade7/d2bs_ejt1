#include "d2bs/script/api/JSProfile.h"

#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

// Profile() - get the active profile
// Profile(name) - get the named profile
//
// Create profiles:
//	Profile(ProfileType.singlePlayer, charname, diff)
//	Profile(ProfileType.battleNet, account, pass, charname, gateway)
//	Profile(ProfileType.openBattleNet, account, pass, charname, gateway)
//	Profile(ProfileType.tcpIpHost, charname, diff)
//	Profile(ProfileType.tcpIpJoin, charname, ip)
CLASS_CTOR(profile) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  d2bs::Profile* prof;
  d2bs::ProfileType pt;
  const wchar_t* str1;
  const wchar_t* str2;
  const wchar_t* str3;
  const wchar_t* str4;

  str1 = str2 = str3 = str4 = NULL;
  pt = d2bs::PROFILETYPE_INVALID;

  try {
    // Profile()
    if (args.length() == 0) {
      if (Vars.settings.szProfile != NULL)
        prof = Vars.settings.get_profile(Vars.settings.szProfile);
      else
        THROW_ERROR(cx, "No active profile!");
    }
    // Profile(name) - get the named profile
    else if (args.length() == 1 && args[0].isString()) {
      str1 = JS_GetStringCharsZ(cx, args[0].toString());
      prof = Vars.settings.get_profile(str1);
    }
    // Profile(ProfileType.singlePlayer, charname, diff)
    else if (args.length() == 3 && args[0].isInt32() &&
             args[0].toInt32() == d2bs::PROFILETYPE_SINGLEPLAYER) {
      // JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "isu", &pt, &i);  //removed s flag no longer supported
      str1 = JS_GetStringCharsZ(cx, args[1].toString());
      prof = Vars.settings.add_profile(d2bs::PROFILETYPE_SINGLEPLAYER, L"", L"", str1, L"",
                                       (char)(args[2].toInt32()));
    }
    // Profile(ProfileType.battleNet, account, pass, charname, gateway)
    else if (args.length() == 5 && args[0].isInt32() &&
             args[0].toInt32() == d2bs::PROFILETYPE_BATTLENET) {
      // JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "issss", &pt, &str1, &str2, &str3, &str4);
      str1 = JS_GetStringCharsZ(cx, args[1].toString());
      str2 = JS_GetStringCharsZ(cx, args[2].toString());
      str3 = JS_GetStringCharsZ(cx, args[3].toString());
      str4 = JS_GetStringCharsZ(cx, args[4].toString());
      prof = Vars.settings.add_profile(d2bs::PROFILETYPE_BATTLENET, str1, str2, str3, str4);
    }
    // Profile(ProfileType.openBattleNet, account, pass, charname, gateway)
    else if (args.length() == 5 && args[0].isInt32() &&
             args[0].toInt32() == d2bs::PROFILETYPE_OPEN_BATTLENET) {
      // JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "issss", &pt, &str1, &str2, &str3, &str4);
      str1 = JS_GetStringCharsZ(cx, args[1].toString());
      str2 = JS_GetStringCharsZ(cx, args[2].toString());
      str3 = JS_GetStringCharsZ(cx, args[3].toString());
      str4 = JS_GetStringCharsZ(cx, args[4].toString());
      prof = Vars.settings.add_profile(d2bs::PROFILETYPE_OPEN_BATTLENET, str1, str2, str3, str4);
    }
    // Profile(ProfileType.tcpIpHost, charname, diff)
    else if (args.length() == 3 && args[0].isInt32() &&
             args[0].toInt32() == d2bs::PROFILETYPE_TCPIP_HOST) {
      // JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "isu", &pt, &str1, &i);
      str1 = JS_GetStringCharsZ(cx, args[1].toString());
      prof = Vars.settings.add_profile(d2bs::PROFILETYPE_TCPIP_HOST, L"", L"", str1, L"",
                                       (char)(args[2].toInt32()));
    }
    // Profile(ProfileType.tcpIpJoin, charname, ip)
    else if (args.length() == 3 && args[0].isInt32() &&
             args[0].toInt32() == d2bs::PROFILETYPE_TCPIP_JOIN) {
      // JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "iss", &pt, &str1, &str2);
      str1 = JS_GetStringCharsZ(cx, args[1].toString());
      str2 = JS_GetStringCharsZ(cx, args[2].toString());
      prof = Vars.settings.add_profile(d2bs::PROFILETYPE_TCPIP_JOIN, L"", L"", str1, str2);
    } else
      THROW_ERROR(cx, "Invalid parameters.");
  } catch (char* ex) {
    THROW_ERROR(cx, ex);
  }

  JSObject* obj = BuildObject(cx, &profile_class, profile_methods, profile_props);
  if (!obj) THROW_ERROR(cx, "Failed to create profile object");
  JS_SetPrivate(obj, prof);

  args.rval().setObjectOrNull(obj);
  return JS_TRUE;
}

JSAPI_FUNC(profile_login) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setUndefined();

  const char* error;
  d2bs::Profile* prof;

  auto self = args.thisv();
  prof = (d2bs::Profile*)JS_GetPrivate(self.toObjectOrNull());

  if (prof->login(&error) != 0) THROW_ERROR(cx, error);

  return JS_TRUE;
}

void profile_finalize(JSFreeOp*, JSObject* obj) {
  // Profile* prof;

  // prof = (Profile*)JS_GetPrivate(obj);

  // if (prof != NULL) delete prof;

  JS_SetPrivate(obj, NULL);
}

JSAPI_PROP(profile_getProperty) {
  d2bs::Profile* prof;

  prof = (d2bs::Profile*)JS_GetPrivate(obj);
  jsval ID;
  JS_IdToValue(cx, id, &ID);

  switch (JSVAL_TO_INT(ID)) {
    case PROFILE_TYPE:
      vp.setInt32(prof->type);
      break;
    case PROFILE_IP:
      vp.setString(JS_NewUCStringCopyZ(cx, prof->ip));
      break;
    case PROFILE_USERNAME:
      vp.setString(JS_NewUCStringCopyZ(cx, prof->username));
      break;
    case PROFILE_GATEWAY:
      vp.setString(JS_NewUCStringCopyZ(cx, prof->gateway));
      break;
    case PROFILE_CHARACTER:
      vp.setString(JS_NewUCStringCopyZ(cx, prof->charname));
      break;
    case PROFILE_DIFFICULTY:
      vp.setInt32(prof->diff);
      break;
    case PROFILE_MAXLOGINTIME:
      vp.setInt32(Vars.settings.maxLoginTime);
      break;
    case PROFILE_MAXCHARSELTIME:
      vp.setInt32(Vars.settings.maxCharTime);
      break;
  }

  return JS_TRUE;
}

EMPTY_CTOR(profileType);

JSAPI_PROP(profileType_getProperty) {
  jsval ID;
  JS_IdToValue(cx, id, &ID);
  vp.set(ID);

  return JS_TRUE;
}
