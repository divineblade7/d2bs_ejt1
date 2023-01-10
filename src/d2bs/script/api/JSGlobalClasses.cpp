#include "d2bs/script/api/JSArea.h"
#include "d2bs/script/api/JSControl.h"
#include "d2bs/script/api/JSDirectory.h"
#include "d2bs/script/api/JSExits.h"
#include "d2bs/script/api/JSFile.h"
#include "d2bs/script/api/JSFileTools.h"
#include "d2bs/script/api/JSParty.h"
#include "d2bs/script/api/JSPresetUnit.h"
#include "d2bs/script/api/JSProfile.h"
#include "d2bs/script/api/JSRoom.h"
#include "d2bs/script/api/JSSQLite.h"
#include "d2bs/script/api/JSSandbox.h"
#include "d2bs/script/api/JSScreenHook.h"
#include "d2bs/script/api/JSScript.h"
#include "d2bs/script/api/JSSocket.h"
#include "d2bs/script/api/JSUnit.h"
#include "d2bs/script/js32.h"

JSClass global_obj = {"global",
                      JSCLASS_GLOBAL_FLAGS,
                      JS_PropertyStub,
                      JS_PropertyStub,
                      JS_PropertyStub,
                      JS_StrictPropertyStub,
                      JS_EnumerateStub,
                      JS_ResolveStub,
                      JS_ConvertStub,
                      NULL,
                      JSCLASS_NO_OPTIONAL_MEMBERS};

JSClass sqlite_db = {"SQLite", JSCLASS_HAS_PRIVATE,
                     JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, (JSPropertyOp)JS_PropertyStub,
                                  (JSStrictPropertyOp)JS_StrictPropertyStub, JS_EnumerateStub, JS_ResolveStub,
                                  JS_ConvertStub, sqlite_finalize, sqlite_ctor)};

JSClass sqlite_stmt = {
    "DBStatement", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, sqlite_stmt_finalize, sqlite_stmt_ctor)};

JSClass script_class = {"D2BSScript", JSCLASS_HAS_PRIVATE,
                        JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                     JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL, script_ctor)};

JSClass frame_class = {"Frame", JSCLASS_HAS_PRIVATE,
                       JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, hook_finalize, frame_ctor)};

JSClass box_class = {"Box", JSCLASS_HAS_PRIVATE,
                     JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, hook_finalize, box_ctor)};

JSClass line_class = {"Line", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, hook_finalize, line_ctor)};

JSClass text_class = {"Text", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, hook_finalize, text_ctor)};

JSClass image_class = {"Image", JSCLASS_HAS_PRIVATE,
                       JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, hook_finalize, image_ctor)};

JSClass sandbox_class = {
    "Sandbox", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(sandbox_addProperty, sandbox_delProperty, sandbox_getProperty, sandbox_setProperty, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, sandbox_finalize, sandbox_ctor)};

JSClass room_class = {"Room", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL, room_ctor)};

JSClass presetunit_class = {
    "PresetUnit", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, presetunit_finalize, presetunit_ctor)};

JSClass party_class = {"Party", JSCLASS_HAS_PRIVATE,
                       JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL, party_ctor)};

JSClass filetools_class = {"FileTools", NULL,
                           JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                        JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL, filetools_ctor)};

JSClass file_class = {"File", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, file_finalize, file_ctor)};

JSClass socket_class = {"Socket", JSCLASS_HAS_PRIVATE,
                        JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                     JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, socket_finalize, socket_ctor)};

JSClass exit_class = {"Exit", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, exit_finalize, exit_ctor)};

JSClass folder_class = {"Folder", JSCLASS_HAS_PRIVATE,
                        JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                     JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, dir_finalize, dir_ctor)};

JSClass control_class = {
    "Control", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, control_finalize, control_ctor)};

JSClass area_class = {"Area", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, area_finalize, area_ctor)};

JSClass unit_class = {"Unit", JSCLASS_HAS_PRIVATE,
                      JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, unit_finalize, unit_ctor)};

JSClass profile_class = {
    "Profile", JSCLASS_HAS_PRIVATE,
    JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
                 JS_ResolveStub, JS_ConvertStub, profile_finalize, profile_ctor)};

JSClass profileType_class = {"ProfileType", JSCLASS_HAS_PRIVATE,
                             JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                          JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL, profileType_ctor)};

JSClass dialogLine_class = {"DailogLine", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
                            JSCLASS_SPEC(JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
                                         JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL, NULL)};

JSClassSpec global_classes[] = {
    {&unit_class, 0, unit_ctor, 0, unit_methods, unit_props, NULL, NULL},
    {&presetunit_class, 0, presetunit_ctor, 0, NULL, presetunit_props, NULL, NULL},
    {&area_class, 0, area_ctor, 0, NULL, area_props, NULL, NULL},
    {&control_class, 0, control_ctor, 0, control_funcs, control_props, NULL, NULL},
    {&folder_class, 0, dir_ctor, 0, dir_methods, dir_props, NULL, NULL},
    {&exit_class, 0, exit_ctor, 0, NULL, exit_props, NULL, NULL},
    {&party_class, 0, party_ctor, 0, party_methods, party_props, NULL, NULL},
    {&room_class, 0, room_ctor, 0, room_methods, room_props, NULL, NULL},

    {&file_class, 0, file_ctor, 0, file_methods, file_props, file_s_methods, NULL},
    {&socket_class, 0, socket_ctor, 0, socket_methods, socket_props, socket_s_methods, NULL},
    {&filetools_class, 0, filetools_ctor, 0, NULL, NULL, filetools_s_methods, NULL},
    {&sqlite_db, 0, sqlite_ctor, 0, sqlite_methods, sqlite_props, NULL, NULL},
    {&sqlite_stmt, 0, sqlite_stmt_ctor, 0, sqlite_stmt_methods, sqlite_stmt_props, NULL, NULL},
    {&sandbox_class, 0, sandbox_ctor, 0, sandbox_methods, NULL, NULL, NULL},
    {&script_class, 0, script_ctor, 0, script_methods, script_props, NULL, NULL},

    {&frame_class, 0, frame_ctor, 0, frame_methods, frame_props, NULL, NULL},
    {&box_class, 0, box_ctor, 0, box_methods, box_props, NULL, NULL},
    {&line_class, 0, line_ctor, 0, line_methods, line_props, NULL, NULL},
    {&text_class, 0, text_ctor, 0, text_methods, text_props, NULL, NULL},
    {&image_class, 0, image_ctor, 0, image_methods, image_props, NULL, NULL},
    {&profile_class, 0, profile_ctor, 0, profile_methods, profile_props, NULL, NULL},
    {0}
};
