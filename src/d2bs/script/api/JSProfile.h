#pragma once

#include "d2bs/new_core/settings.h"
#include "d2bs/script/js32.h"

#include <windows.h>

struct jsProfilePrivate {
  char* password;
};

CLASS_CTOR(profile);
void profile_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(profile_type);
JSAPI_PROP(profile_id);
JSAPI_PROP(profile_username);
JSAPI_PROP(profile_gateway);
JSAPI_PROP(profile_character);
JSAPI_PROP(profile_difficulty);
JSAPI_PROP(profile_maxLoginTime);
JSAPI_PROP(profile_maxCharacterSelectTime);

JSAPI_FUNC(profile_login);

// clang-format off
static JSPropertySpec profile_props[] = {
  JS_PSG("type",                      profile_type,                     JSPROP_PERMANENT_VAR),
  JS_PSG("ip",                        profile_id,                       JSPROP_PERMANENT_VAR),
  JS_PSG("username",                  profile_username,                 JSPROP_PERMANENT_VAR),
  JS_PSG("gateway",                   profile_gateway,                  JSPROP_PERMANENT_VAR),
  JS_PSG("character",                 profile_character,                JSPROP_PERMANENT_VAR),
  JS_PSG("difficulty",                profile_difficulty,               JSPROP_PERMANENT_VAR),
  JS_PSG("maxLoginTime",              profile_maxLoginTime,             JSPROP_PERMANENT_VAR),
  JS_PSG("maxCharacterSelectTime",    profile_maxCharacterSelectTime,   JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec profile_methods[] = {
  JS_FS("login", profile_login, 0, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass profile_class{
    "Profile",                              // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    profile_finalize,       // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    profile_ctor,           // construct
                    nullptr)                // trace
};

CLASS_CTOR(profileType);

JSAPI_PROP(profileType_getProperty);

// clang-format off
static JSPropertySpec profileType_props[] = {
  JS_PS("singlePlayer",   d2bs::PROFILETYPE_SINGLEPLAYER,   JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER),
  JS_PS("battleNet",      d2bs::PROFILETYPE_BATTLENET,      JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER),
  JS_PS("openBattleNet",  d2bs::PROFILETYPE_OPEN_BATTLENET, JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER),
  JS_PS("tcpIpHost",      d2bs::PROFILETYPE_TCPIP_HOST,     JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER),
  JS_PS("tcpIpJoin",      d2bs::PROFILETYPE_TCPIP_JOIN,     JSPROP_PERMANENT_VAR, JSOP_WRAPPER(profileType_getProperty), JSOP_NULLWRAPPER),
  JS_PS_END
};
// clang-format on

static JSClass profileType_class{
    "ProfileType",                          // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    nullptr,                // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    profileType_ctor,       // construct
                    nullptr)                // trace
};
