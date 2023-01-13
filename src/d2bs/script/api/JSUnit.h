#pragma once

#include "d2bs/script/js32.h"
#include "d2bs/utils/Offset.h"

#include <windows.h>

struct myUnit {
  DWORD _dwPrivateType;
  DWORD dwUnitId;
  DWORD dwClassId;
  DWORD dwType;
  DWORD dwMode;
  char szName[128]{};
};

struct invUnit {
  DWORD _dwPrivateType;
  DWORD dwUnitId;
  DWORD dwClassId;
  DWORD dwType;
  DWORD dwMode;
  char szName[128]{};
  DWORD dwOwnerId;
  DWORD dwOwnerType;
};

CLASS_CTOR(unit);
void unit_finalize(JSFreeOp* fop, JSObject* obj);

JSAPI_PROP(me_account);
JSAPI_PROP(me_charname);
JSAPI_PROP(me_diff);
JSAPI_PROP(me_maxdiff);
JSAPI_PROP(me_gamename);
JSAPI_PROP(me_gamepassword);
JSAPI_PROP(me_gameserverip);
JSAPI_PROP(me_gamestarttime);
JSAPI_PROP(me_gametype);
JSAPI_PROP(me_itemoncursor);
JSAPI_PROP(me_automap);
JSAPI_STRICT_PROP(me_automap_setter);
JSAPI_PROP(me_ladder);
JSAPI_PROP(me_ping);
JSAPI_PROP(me_fps);
JSAPI_PROP(me_locale);
JSAPI_PROP(me_playertype);
JSAPI_PROP(me_realm);
JSAPI_PROP(me_realmshort);
JSAPI_PROP(me_mercrevivecost);
JSAPI_PROP(me_runwalk);
JSAPI_STRICT_PROP(me_runwalk_setter);
JSAPI_PROP(me_weaponswitch);
JSAPI_PROP(me_chickenhp);
JSAPI_STRICT_PROP(me_chickenhp_setter);
JSAPI_PROP(me_chickenmp);
JSAPI_STRICT_PROP(me_chickenmp_setter);
JSAPI_PROP(me_quitonhostile);
JSAPI_STRICT_PROP(me_quitonhostile_setter);
JSAPI_PROP(me_blockKeys);
JSAPI_STRICT_PROP(me_blockKeys_setter);
JSAPI_PROP(me_blockMouse);
JSAPI_STRICT_PROP(me_blockMouse_setter);
JSAPI_PROP(me_gameReady);
JSAPI_PROP(me_profile);
JSAPI_PROP(me_nopickup);
JSAPI_STRICT_PROP(me_nopickup_setter);
JSAPI_PROP(me_pid);
JSAPI_PROP(me_unsupported);
JSAPI_PROP(me_charflags);
JSAPI_PROP(me_mapid);

JSAPI_PROP(oog_screensize);
JSAPI_PROP(oog_windowtitle);
JSAPI_PROP(oog_ingame);
JSAPI_PROP(oog_quitonerror);
JSAPI_STRICT_PROP(oog_quitonerror_setter);
JSAPI_PROP(oog_maxgametime);
JSAPI_STRICT_PROP(oog_maxgametime_setter);

JSAPI_PROP(unit_type);
JSAPI_PROP(unit_classid);
JSAPI_PROP(unit_mode);
JSAPI_PROP(unit_name);
JSAPI_PROP(unit_act);
JSAPI_PROP(unit_gid);
JSAPI_PROP(unit_x);
JSAPI_PROP(unit_y);
JSAPI_PROP(unit_targetx);
JSAPI_PROP(unit_targety);
JSAPI_PROP(unit_area);
JSAPI_PROP(unit_hp);
JSAPI_PROP(unit_hpmax);
JSAPI_PROP(unit_mp);
JSAPI_PROP(unit_mpmax);
JSAPI_PROP(unit_stamina);
JSAPI_PROP(unit_staminamax);
JSAPI_PROP(unit_charlvl);
JSAPI_PROP(unit_itemcount);
JSAPI_PROP(unit_owner);
JSAPI_PROP(unit_ownertype);
JSAPI_PROP(unit_spectype);
JSAPI_PROP(unit_direction);
JSAPI_PROP(unit_uniqueid);

JSAPI_PROP(item_code);
JSAPI_PROP(item_prefix);
JSAPI_PROP(item_suffix);
JSAPI_PROP(item_prefixes);
JSAPI_PROP(item_suffixes);
JSAPI_PROP(item_prefixnum);
JSAPI_PROP(item_suffixnum);
JSAPI_PROP(item_prefixnums);
JSAPI_PROP(item_suffixnums);
JSAPI_PROP(item_fname);
JSAPI_PROP(item_quality);
JSAPI_PROP(item_node);
JSAPI_PROP(item_location);
JSAPI_PROP(item_sizex);
JSAPI_PROP(item_sizey);
JSAPI_PROP(item_type);
JSAPI_PROP(item_description);
JSAPI_PROP(item_bodylocation);
JSAPI_PROP(item_ilvl);
JSAPI_PROP(item_levelreq);
JSAPI_PROP(item_gfx);

JSAPI_PROP(object_type);
JSAPI_PROP(object_locked);

JSAPI_FUNC(unit_getUnit);
JSAPI_FUNC(unit_getNext);
JSAPI_FUNC(unit_cancel);
JSAPI_FUNC(unit_repair);
JSAPI_FUNC(unit_useMenu);
JSAPI_FUNC(unit_interact);
JSAPI_FUNC(unit_getStat);
JSAPI_FUNC(unit_getState);
JSAPI_FUNC(unit_getItems);
JSAPI_FUNC(unit_getSkill);
JSAPI_FUNC(unit_getParent);
JSAPI_FUNC(unit_setskill);
JSAPI_FUNC(unit_getMerc);
JSAPI_FUNC(unit_getMercHP);
JSAPI_FUNC(unit_getItem);
JSAPI_FUNC(unit_move);
JSAPI_FUNC(item_getFlag);
JSAPI_FUNC(item_getFlags);
// JSAPI_FUNC(item_getPrice);
JSAPI_FUNC(item_shop);
JSAPI_FUNC(my_overhead);
JSAPI_FUNC(my_revive);
JSAPI_FUNC(unit_getEnchant);
JSAPI_FUNC(unit_getQuest);
JSAPI_FUNC(unit_getMinionCount);
JSAPI_FUNC(me_getRepairCost);
JSAPI_FUNC(item_getItemCost);

// clang-format off
static JSPropertySpec me_props[] = {
  JS_PSG("account",             me_account,             JSPROP_PERMANENT_VAR),
  JS_PSG("charname",            me_charname,            JSPROP_PERMANENT_VAR),
  JS_PSG("diff",                me_diff,                JSPROP_PERMANENT_VAR),
  JS_PSG("maxdiff",             me_maxdiff,             JSPROP_PERMANENT_VAR),
  JS_PSG("gamename",            me_gamename,            JSPROP_PERMANENT_VAR),
  JS_PSG("gamepassword",        me_gamepassword,        JSPROP_PERMANENT_VAR),
  JS_PSG("gameserverip",        me_gameserverip,        JSPROP_PERMANENT_VAR),
  JS_PSG("gamestarttime",       me_gamestarttime,       JSPROP_PERMANENT_VAR),
  JS_PSG("gametype",            me_gametype,            JSPROP_PERMANENT_VAR),
  JS_PSG("itemoncursor",        me_itemoncursor,        JSPROP_PERMANENT_VAR),
  JS_PSGS("automap",            me_automap,             me_automap_setter,          JSPROP_STATIC_VAR),
  JS_PSG("ladder",              me_ladder,              JSPROP_PERMANENT_VAR),
  JS_PSG("ping",                me_ping,                JSPROP_PERMANENT_VAR),
  JS_PSG("fps",                 me_fps,                 JSPROP_PERMANENT_VAR),
  JS_PSG("locale",              me_locale,              JSPROP_PERMANENT_VAR),
  JS_PSG("playertype",          me_playertype,          JSPROP_PERMANENT_VAR),
  JS_PSG("realm",               me_realm,               JSPROP_PERMANENT_VAR),
  JS_PSG("realmshort",          me_realmshort,          JSPROP_PERMANENT_VAR),
  JS_PSG("mercrevivecost",      me_mercrevivecost,      JSPROP_PERMANENT_VAR),
  JS_PSGS("runwalk",            me_runwalk,             me_runwalk_setter,          JSPROP_STATIC_VAR),
  JS_PSG("weaponswitch",        me_weaponswitch,        JSPROP_PERMANENT_VAR),
  JS_PSGS("chickenhp",          me_chickenhp,           me_chickenhp_setter,        JSPROP_STATIC_VAR),
  JS_PSGS("chickenmp",          me_chickenmp,           me_chickenmp_setter,        JSPROP_STATIC_VAR),
  JS_PSGS("quitonhostile",      me_quitonhostile,       me_quitonhostile_setter,    JSPROP_STATIC_VAR),
  JS_PSGS("blockKeys",          me_blockKeys,           me_blockKeys_setter,        JSPROP_STATIC_VAR),
  JS_PSGS("blockMouse",         me_blockMouse,          me_blockMouse_setter,       JSPROP_STATIC_VAR),
  JS_PSG("gameReady",           me_gameReady,           JSPROP_PERMANENT_VAR),
  JS_PSG("profile",             me_profile,             JSPROP_PERMANENT_VAR),
  JS_PSGS("nopickup",           me_nopickup,            me_nopickup_setter,         JSPROP_STATIC_VAR),
  JS_PSG("pid",                 me_pid,                 JSPROP_PERMANENT_VAR),
  JS_PSG("unsupported",         me_unsupported,         JSPROP_PERMANENT_VAR),
  JS_PSG("charflags",           me_charflags,           JSPROP_PERMANENT_VAR),
  JS_PSG("mapid",               me_mapid,               JSPROP_PERMANENT_VAR),

  JS_PSG("screensize",          oog_screensize,         JSPROP_PERMANENT_VAR),
  JS_PSG("windowtitle",         oog_windowtitle,        JSPROP_PERMANENT_VAR),
  JS_PSG("ingame",              oog_ingame,             JSPROP_PERMANENT_VAR),
  JS_PSGS("quitonerror",        oog_quitonerror,        oog_quitonerror_setter,     JSPROP_STATIC_VAR),
  JS_PSGS("maxgametime",        oog_maxgametime,        oog_maxgametime_setter,     JSPROP_STATIC_VAR),

  JS_PSG("type",                unit_type,              JSPROP_PERMANENT_VAR),
  JS_PSG("classid",             unit_classid,           JSPROP_PERMANENT_VAR),
  JS_PSG("mode",                unit_mode,              JSPROP_PERMANENT_VAR),
  JS_PSG("name",                unit_name,              JSPROP_PERMANENT_VAR),
  JS_PSG("act",                 unit_act,               JSPROP_PERMANENT_VAR),
  JS_PSG("gid",                 unit_gid,               JSPROP_PERMANENT_VAR),
  JS_PSG("x",                   unit_x,                 JSPROP_PERMANENT_VAR),
  JS_PSG("y",                   unit_y,                 JSPROP_PERMANENT_VAR),
  JS_PSG("targetx",             unit_targetx,           JSPROP_PERMANENT_VAR),
  JS_PSG("targety",             unit_targety,           JSPROP_PERMANENT_VAR),
  JS_PSG("area",                unit_area,              JSPROP_PERMANENT_VAR),
  JS_PSG("hp",                  unit_hp,                JSPROP_PERMANENT_VAR),
  JS_PSG("hpmax",               unit_hpmax,             JSPROP_PERMANENT_VAR),
  JS_PSG("mp",                  unit_mp,                JSPROP_PERMANENT_VAR),
  JS_PSG("mpmax",               unit_mpmax,             JSPROP_PERMANENT_VAR),
  JS_PSG("stamina",             unit_stamina,           JSPROP_PERMANENT_VAR),
  JS_PSG("staminamax",          unit_staminamax,        JSPROP_PERMANENT_VAR),
  JS_PSG("charlvl",             unit_charlvl,           JSPROP_PERMANENT_VAR),
  JS_PSG("itemcount",           unit_itemcount,         JSPROP_PERMANENT_VAR),
  JS_PSG("owner",               unit_owner,             JSPROP_PERMANENT_VAR),
  JS_PSG("ownertype",           unit_ownertype,         JSPROP_PERMANENT_VAR),
  JS_PSG("spectype",            unit_spectype,          JSPROP_PERMANENT_VAR),
  JS_PSG("direction",           unit_direction,         JSPROP_PERMANENT_VAR),

  JS_PSG("code",                item_code,              JSPROP_PERMANENT_VAR),
  JS_PSG("prefix",              item_prefix,            JSPROP_PERMANENT_VAR),
  JS_PSG("suffix",              item_suffix,            JSPROP_PERMANENT_VAR),
  JS_PSG("prefixes",            item_prefixes,          JSPROP_PERMANENT_VAR),
  JS_PSG("suffixes",            item_suffixes,          JSPROP_PERMANENT_VAR),
  JS_PSG("prefixnum",           item_prefixnum,         JSPROP_PERMANENT_VAR),
  JS_PSG("suffixnum",           item_suffixnum,         JSPROP_PERMANENT_VAR),
  JS_PSG("prefixnums",          item_prefixnums,        JSPROP_PERMANENT_VAR),
  JS_PSG("suffixnums",          item_suffixnums,        JSPROP_PERMANENT_VAR),
  JS_PSG("fname",               item_fname,             JSPROP_PERMANENT_VAR),
  JS_PSG("quality",             item_quality,           JSPROP_PERMANENT_VAR),
  JS_PSG("node",                item_node,              JSPROP_PERMANENT_VAR),
  JS_PSG("location",            item_location,          JSPROP_PERMANENT_VAR),
  JS_PSG("sizex",               item_sizex,             JSPROP_PERMANENT_VAR),
  JS_PSG("sizey",               item_sizey,             JSPROP_PERMANENT_VAR),
  JS_PSG("itemType",            item_type,              JSPROP_PERMANENT_VAR),
  JS_PSG("description",         item_description,       JSPROP_PERMANENT_VAR),
  JS_PSG("bodylocation",        item_bodylocation,      JSPROP_PERMANENT_VAR),
  JS_PSG("ilvl",                item_ilvl,              JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSPropertySpec unit_props[] = {
  // why??
  JS_PSG("runwalk",             me_runwalk,             JSPROP_PERMANENT_VAR),
  JS_PSG("weaponswitch",        me_weaponswitch,        JSPROP_PERMANENT_VAR),

  JS_PSG("type",                unit_type,              JSPROP_PERMANENT_VAR),
  JS_PSG("classid",             unit_classid,           JSPROP_PERMANENT_VAR),
  JS_PSG("mode",                unit_mode,              JSPROP_PERMANENT_VAR),
  JS_PSG("name",                unit_name,              JSPROP_PERMANENT_VAR),
  JS_PSG("act",                 unit_act,               JSPROP_PERMANENT_VAR),
  JS_PSG("gid",                 unit_gid,               JSPROP_PERMANENT_VAR),
  JS_PSG("x",                   unit_x,                 JSPROP_PERMANENT_VAR),
  JS_PSG("y",                   unit_y,                 JSPROP_PERMANENT_VAR),
  JS_PSG("targetx",             unit_targetx,           JSPROP_PERMANENT_VAR),
  JS_PSG("targety",             unit_targety,           JSPROP_PERMANENT_VAR),
  JS_PSG("area",                unit_area,              JSPROP_PERMANENT_VAR),
  JS_PSG("hp",                  unit_hp,                JSPROP_PERMANENT_VAR),
  JS_PSG("hpmax",               unit_hpmax,             JSPROP_PERMANENT_VAR),
  JS_PSG("mp",                  unit_mp,                JSPROP_PERMANENT_VAR),
  JS_PSG("mpmax",               unit_mpmax,             JSPROP_PERMANENT_VAR),
  JS_PSG("stamina",             unit_stamina,           JSPROP_PERMANENT_VAR),
  JS_PSG("staminamax",          unit_staminamax,        JSPROP_PERMANENT_VAR),
  JS_PSG("charlvl",             unit_charlvl,           JSPROP_PERMANENT_VAR),
  JS_PSG("itemcount",           unit_itemcount,         JSPROP_PERMANENT_VAR),
  JS_PSG("owner",               unit_owner,             JSPROP_PERMANENT_VAR),
  JS_PSG("ownertype",           unit_ownertype,         JSPROP_PERMANENT_VAR),
  JS_PSG("spectype",            unit_spectype,          JSPROP_PERMANENT_VAR),
  JS_PSG("direction",           unit_direction,         JSPROP_PERMANENT_VAR),
  JS_PSG("uniqueid",            unit_uniqueid,          JSPROP_PERMANENT_VAR),

  JS_PSG("code",                item_code,              JSPROP_PERMANENT_VAR),
  JS_PSG("prefix",              item_prefix,            JSPROP_PERMANENT_VAR),
  JS_PSG("suffix",              item_suffix,            JSPROP_PERMANENT_VAR),
  JS_PSG("prefixes",            item_prefixes,          JSPROP_PERMANENT_VAR),
  JS_PSG("suffixes",            item_suffixes,          JSPROP_PERMANENT_VAR),
  JS_PSG("prefixnum",           item_prefixnum,         JSPROP_PERMANENT_VAR),
  JS_PSG("suffixnum",           item_suffixnum,         JSPROP_PERMANENT_VAR),
  JS_PSG("prefixnums",          item_prefixnums,        JSPROP_PERMANENT_VAR),
  JS_PSG("suffixnums",          item_suffixnums,        JSPROP_PERMANENT_VAR),
  JS_PSG("fname",               item_fname,             JSPROP_PERMANENT_VAR),
  JS_PSG("quality",             item_quality,           JSPROP_PERMANENT_VAR),
  JS_PSG("node",                item_node,              JSPROP_PERMANENT_VAR),
  JS_PSG("location",            item_location,          JSPROP_PERMANENT_VAR),
  JS_PSG("sizex",               item_sizex,             JSPROP_PERMANENT_VAR),
  JS_PSG("sizey",               item_sizey,             JSPROP_PERMANENT_VAR),
  JS_PSG("itemType",            item_type,              JSPROP_PERMANENT_VAR),
  JS_PSG("description",         item_description,       JSPROP_PERMANENT_VAR),
  JS_PSG("bodylocation",        item_bodylocation,      JSPROP_PERMANENT_VAR),
  JS_PSG("ilvl",                item_ilvl,              JSPROP_PERMANENT_VAR),
  JS_PSG("lvlreq",              item_levelreq,          JSPROP_PERMANENT_VAR),
  JS_PSG("gfx",                 item_gfx,               JSPROP_PERMANENT_VAR),

  JS_PSG("objtype",             object_type,            JSPROP_PERMANENT_VAR),
  JS_PSG("islocked",            object_locked,          JSPROP_PERMANENT_VAR),
  JS_PS_END
};

static JSFunctionSpec unit_methods[] = {
  JS_FS("getNext",              unit_getNext,           0, FUNCTION_FLAGS),
  JS_FS("cancel",               unit_cancel,            0, FUNCTION_FLAGS),
  JS_FS("repair",               unit_repair,            0, FUNCTION_FLAGS),
  JS_FS("useMenu",              unit_useMenu,           0, FUNCTION_FLAGS),
  JS_FS("interact",             unit_interact,          0, FUNCTION_FLAGS),
  JS_FS("getItem",              unit_getItem,           3, FUNCTION_FLAGS),
  JS_FS("getItems",             unit_getItems,          0, FUNCTION_FLAGS),
  JS_FS("getMerc",              unit_getMerc,           0, FUNCTION_FLAGS),
  JS_FS("getMercHP",            unit_getMercHP,         0, FUNCTION_FLAGS),
  JS_FS("getSkill",             unit_getSkill,          0, FUNCTION_FLAGS),
  JS_FS("getParent",            unit_getParent,         0, FUNCTION_FLAGS),
  JS_FS("overhead",             my_overhead,            0, FUNCTION_FLAGS),
  JS_FS("revive",               my_revive,              0, FUNCTION_FLAGS),
  JS_FS("getFlags",             item_getFlags,          1, FUNCTION_FLAGS),
  JS_FS("getFlag",              item_getFlag,           1, FUNCTION_FLAGS),
  JS_FS("getStat",              unit_getStat,           1, FUNCTION_FLAGS),
  JS_FS("getState",             unit_getState,          1, FUNCTION_FLAGS),
  JS_FS("getEnchant",           unit_getEnchant,        1, FUNCTION_FLAGS),
  JS_FS("shop",                 item_shop,              2, FUNCTION_FLAGS),
  JS_FS("setSkill",             unit_setskill,          2, FUNCTION_FLAGS),
  JS_FS("move",                 unit_move,              2, FUNCTION_FLAGS),
  JS_FS("getQuest",             unit_getQuest,          2, FUNCTION_FLAGS),
  JS_FS("getMinionCount",       unit_getMinionCount,    1, FUNCTION_FLAGS),
  JS_FS("getRepairCost",        me_getRepairCost,       1, FUNCTION_FLAGS),
  JS_FS("getItemCost",          item_getItemCost,       1, FUNCTION_FLAGS),
  JS_FS_END
};
// clang-format on

static JSClass unit_class{
    "Unit",                                 // name
    JSCLASS_HAS_PRIVATE,                    // flags
    JSCLASS_METHODS(JS_PropertyStub,        // addProperty
                    JS_PropertyStub,        // delProperty
                    JS_PropertyStub,        // getProperty
                    JS_StrictPropertyStub,  // setProperty
                    JS_EnumerateStub,       // enumerate
                    JS_ResolveStub,         // resolve
                    JS_ConvertStub,         // mayResolve
                    unit_finalize,          // finalize
                    nullptr,                // call
                    nullptr,                // hasInstance
                    unit_ctor,              // construct
                    nullptr)                // trace
};
