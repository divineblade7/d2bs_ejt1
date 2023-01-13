#pragma once

#include "d2bs/script/api/JSGlobalClasses.h"
#include "d2bs/script/js32.h"

JSAPI_FUNC(my_rand);
JSAPI_FUNC(my_submitItem);
JSAPI_FUNC(my_copyUnit);
JSAPI_FUNC(my_clickMap);
JSAPI_FUNC(my_acceptTrade);
JSAPI_FUNC(my_tradeOk);
JSAPI_FUNC(my_getDialogLines);
JSAPI_FUNC(my_clickDialog);
JSAPI_FUNC(my_clickItem);
JSAPI_FUNC(my_gold);
JSAPI_FUNC(my_checkCollision);
JSAPI_FUNC(my_playSound);
JSAPI_FUNC(my_quit);
JSAPI_FUNC(my_quitGame);
JSAPI_FUNC(my_say);
JSAPI_FUNC(my_weaponSwitch);
JSAPI_FUNC(my_transmute);
JSAPI_FUNC(my_clickParty);
JSAPI_FUNC(my_useStatPoint);
JSAPI_FUNC(my_useSkillPoint);

JSAPI_FUNC(my_getInteractedNPC);
JSAPI_FUNC(my_getIsTalkingNPC);

JSAPI_FUNC(my_takeScreenshot);
JSAPI_FUNC(my_getMouseCoords);
JSAPI_FUNC(my_getDistance);
JSAPI_FUNC(my_getPath);
JSAPI_FUNC(my_getCollision);
JSAPI_FUNC(unit_getMercHP);
JSAPI_FUNC(my_getCursorType);
JSAPI_FUNC(my_getSkillByName);
JSAPI_FUNC(my_getSkillById);
JSAPI_FUNC(my_getLocaleString);
JSAPI_FUNC(my_getTextSize);
JSAPI_FUNC(my_getUIFlag);
JSAPI_FUNC(my_getTradeInfo);
JSAPI_FUNC(my_getWaypoint);
JSAPI_FUNC(my_getBaseStat);
JSAPI_FUNC(my_getPlayerFlag);
JSAPI_FUNC(my_moveNPC);
JSAPI_FUNC(my_revealLevel);

static JSClass dialogLine_class{
    // TODO: yes this is mispelled, wont fix until I can figure out if it's used in kolbot ~ ejt
    "DailogLine",                                         // name
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),  // flags
    JSCLASS_METHODS(JS_PropertyStub,                      // addProperty
                    JS_PropertyStub,                      // delProperty
                    JS_PropertyStub,                      // getProperty
                    JS_StrictPropertyStub,                // setProperty
                    JS_EnumerateStub,                     // enumerate
                    JS_ResolveStub,                       // resolve
                    JS_ConvertStub,                       // mayResolve
                    nullptr,                              // finalize
                    nullptr,                              // call
                    nullptr,                              // hasInstance
                    nullptr,                              // construct
                    nullptr)                              // trace
};
