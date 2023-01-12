#pragma once

#include "d2bs/new_core/settings.h"
#include "d2bs/script/js32.h"

#include <windows.h>

CLASS_CTOR(profile);

JSAPI_FUNC(profile_login);

void profile_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(profile_getProperty);

struct jsProfilePrivate {
  char* password;
};

enum jsProfileProperty_ids {
  PROFILE_TYPE,
  PROFILE_IP,
  PROFILE_USERNAME,
  PROFILE_GATEWAY,
  PROFILE_CHARACTER,
  PROFILE_DIFFICULTY,
  PROFILE_MAXLOGINTIME,
  PROFILE_MAXCHARSELTIME
};

static JSPropertySpec profile_props[] = {
    {"type", PROFILE_TYPE, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"ip", PROFILE_IP, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"username", PROFILE_USERNAME, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"gateway", PROFILE_GATEWAY, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"character", PROFILE_CHARACTER, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"difficulty", PROFILE_DIFFICULTY, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"maxLoginTime", PROFILE_MAXLOGINTIME, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty), JSOP_NULLWRAPPER},
    {"maxCharacterSelectTime", PROFILE_MAXCHARSELTIME, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profile_getProperty),
     JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}};

static JSFunctionSpec profile_methods[] = {JS_FS("login", profile_login, 0, FUNCTION_FLAGS), JS_FS_END};

CLASS_CTOR(profileType);

JSAPI_PROP(profileType_getProperty);

static JSPropertySpec profileType_props[] = {{"singlePlayer", d2bs::PROFILETYPE_SINGLEPLAYER, JSPROP_PERMANENT_VAR,
                                              JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER},
                                             {"battleNet", d2bs::PROFILETYPE_BATTLENET, JSPROP_PERMANENT_VAR,
                                              JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER},
                                             {"openBattleNet", d2bs::PROFILETYPE_OPEN_BATTLENET, JSPROP_PERMANENT_VAR,
                                              JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER},
                                             {"tcpIpHost", d2bs::PROFILETYPE_TCPIP_HOST, JSPROP_PERMANENT_VAR,
                                              JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER},
                                             {"tcpIpJoin", d2bs::PROFILETYPE_TCPIP_JOIN, JSPROP_PERMANENT_VAR,
                                              JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER},
                                             {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}};

static JSClass profile_class = {
    "Profile", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, profile_finalize, profile_ctor)};

static JSClass profileType_class = {
    "ProfileType", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, NULL, profileType_ctor)};
