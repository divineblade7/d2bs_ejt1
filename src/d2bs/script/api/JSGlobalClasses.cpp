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

JSClassSpec global_classes[] = {
    {&unit_class, 0, 0, unit_methods, unit_props, NULL, NULL},
    {&presetunit_class, 0, 0, NULL, presetunit_props, NULL, NULL},
    {&area_class, 0, 0, NULL, area_props, NULL, NULL},
    {&control_class, 0, 0, control_funcs, control_props, NULL, NULL},
    {&folder_class, 0, 0, dir_methods, dir_props, NULL, NULL},
    {&exit_class, 0, 0, NULL, exit_props, NULL, NULL},
    {&party_class, 0, 0, party_methods, party_props, NULL, NULL},
    {&room_class, 0, 0, room_methods, room_props, NULL, NULL},

    {&file_class, 0, 0, file_methods, file_props, file_s_methods, NULL},
    {&socket_class, 0, 0, socket_methods, socket_props, socket_s_methods, NULL},
    {&filetools_class, 0, 0, NULL, NULL, filetools_s_methods, NULL},
    {&sqlite_db, 0, 0, sqlite_methods, sqlite_props, NULL, NULL},
    {&sqlite_stmt, 0, 0, sqlite_stmt_methods, sqlite_stmt_props, NULL, NULL},
    {&sandbox_class, 0, 0, sandbox_methods, NULL, NULL, NULL},
    {&script_class, 0, 0, script_methods, script_props, NULL, NULL},

    {&frame_class, 0, 0, frame_methods, frame_props, NULL, NULL},
    {&box_class, 0, 0, box_methods, box_props, NULL, NULL},
    {&line_class, 0, 0, line_methods, line_props, NULL, NULL},
    {&text_class, 0, 0, text_methods, text_props, NULL, NULL},
    {&image_class, 0, 0, image_methods, image_props, NULL, NULL},
    {&profile_class, 0, 0, profile_methods, profile_props, NULL, NULL},
    {0}};
