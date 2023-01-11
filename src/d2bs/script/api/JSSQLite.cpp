/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "d2bs/script/api/JSSQLite.h"

#include "d2bs/core/File.h"
#include "d2bs/utils/Helpers.h"
#include "d2bs/variables.h"

#include <set>

struct SqliteDB;
struct DBStmt;
typedef std::set<DBStmt*> StmtList;

struct SqliteDB {
  sqlite3* db;
  bool open;
  std::wstring path;
  StmtList stmts;
};

struct DBStmt {
  DBStmt(JSContext* cx) : current_row(cx) {}
  sqlite3_stmt* stmt;
  bool open, canGet;
  SqliteDB* assoc_db;
  JS::RootedValue current_row;
};

void close_db_stmt(DBStmt* stmt);
bool clean_sqlite_db(SqliteDB* db);

bool clean_sqlite_db(SqliteDB* db) {
  if (db && db->open) {
    for (StmtList::iterator it = db->stmts.begin(); it != db->stmts.end(); it++) {
      close_db_stmt(*it);
    }
    db->stmts.clear();
    if (SQLITE_OK != sqlite3_close(db->db)) return false;
  }
  return true;
}

void close_db_stmt(DBStmt* stmt) {
  if (stmt->stmt && stmt->open) {
    sqlite3_finalize(stmt->stmt);
    stmt->stmt = NULL;
    stmt->open = false;
  }
}

JSAPI_FUNC(my_sqlite_version) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setString(JS_InternString(cx, sqlite3_version));
  return JS_TRUE;
}

JSAPI_FUNC(my_sqlite_memusage) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setNumber(static_cast<double>(sqlite3_memory_used()));
  return JS_TRUE;
}

EMPTY_CTOR(sqlite_stmt)

JSAPI_FUNC(sqlite_ctor) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() > 0 && !args[0].isString()) THROW_ERROR(cx, "Invalid parameters in SQLite constructor");
  std::wstring path;

  if (args.length() > 0)
    path = JS_GetStringCharsZ(cx, args[0].toString());
  else
    path = L":memory:";

  // if the path is not a special placeholder (:memory:, et. al.), sandbox it
  if (path[0] != ':') {
    if (!isValidPath(path.c_str())) {
      THROW_ERROR(cx, "Invalid characters in database name");
    }

    path = (Vars.settings.script_dir / path).make_preferred().wstring();
  }

  bool autoOpen = true;
  if (args.get(1).isBoolean()) autoOpen = args[1].toBoolean();

  sqlite3* db = NULL;
  if (autoOpen) {
    if (SQLITE_OK != sqlite3_open16(path.c_str(), &db)) {
      char msg[1024];
      sprintf_s(msg, sizeof(msg), "Could not open database: %s", sqlite3_errmsg(db));
      THROW_ERROR(cx, msg);
    }
  }

  SqliteDB* dbobj = new SqliteDB;  // leaked?
  dbobj->db = db;
  dbobj->open = autoOpen;
  dbobj->path = path;

  JSObject* jsdb = BuildObject(cx, &sqlite_db, sqlite_methods, sqlite_props, dbobj);
  if (!jsdb) {
    sqlite3_close(db);
    delete dbobj;
    THROW_ERROR(cx, "Could not create the sqlite object");
  }

  args.rval().setObjectOrNull(jsdb);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_execute) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SqliteDB* dbobj = (SqliteDB*)JS_GetInstancePrivate(cx, self, &sqlite_db, NULL);
  if (dbobj->open != true) THROW_ERROR(cx, "Database must first be opened!");
  if (args.length() < 1 || args.length() > 1 || !args[0].isString())
    THROW_ERROR(cx, "Invalid parameters in SQLite.execute");

  char *sql = JS_EncodeStringToUTF8(cx, args[0].toString()), *err = NULL;
  if (SQLITE_OK != sqlite3_exec(dbobj->db, sql, NULL, NULL, &err)) {
    char msg[2048];
    strcpy_s(msg, sizeof(msg), err);
    sqlite3_free(err);
    JS_free(cx, sql);
    THROW_ERROR(cx, msg);
  }
  JS_free(cx, sql);

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_query) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SqliteDB* dbobj = (SqliteDB*)JS_GetInstancePrivate(cx, self, &sqlite_db, NULL);
  if (dbobj->open != true) THROW_ERROR(cx, "Database must first be opened!");
  if (args.length() < 1 || !args[0].isString()) THROW_ERROR(cx, "Invalid parameters to SQLite.query");

  char* sql = JS_EncodeStringToUTF8(cx, args[0].toString());
  sqlite3_stmt* stmt;
  if (SQLITE_OK != sqlite3_prepare_v2(dbobj->db, sql, strlen(sql), &stmt, NULL)) {
    JS_free(cx, sql);
    THROW_ERROR(cx, sqlite3_errmsg(dbobj->db));
  }
  if (stmt == NULL) {
    JS_free(cx, sql);
    THROW_ERROR(cx, "Statement has no effect");
  }

  char* szText;
  for (uint i = 1; i < args.length(); i++) {
    switch (JS_TypeOfValue(cx, args[i])) {
      case JSTYPE_VOID:
        sqlite3_bind_null(stmt, i);
        break;
      case JSTYPE_STRING:
        szText = JS_EncodeStringToUTF8(cx, args[i].toString());
        sqlite3_bind_text(stmt, i, szText, -1, SQLITE_STATIC);
        JS_free(cx, szText);
        break;
      case JSTYPE_NUMBER:
        if (args[i].isNumber())
          sqlite3_bind_double(stmt, i, (jsdouble)args[i].toNumber());
        else if (args[i].isInt32())
          sqlite3_bind_int(stmt, i, args[i].isInt32());
        break;
      case JSTYPE_BOOLEAN:
        sqlite3_bind_text(stmt, i, args[i].toBoolean() ? "true" : "false", -1, SQLITE_STATIC);
        break;
      default:
        sqlite3_finalize(stmt);
        char msg[1024];
        sprintf_s(msg, sizeof(msg), "Invalid bound parameter %i", i);
        JS_free(cx, sql);
        THROW_ERROR(cx, msg);
        break;
    }
  }

  DBStmt* dbstmt = new DBStmt(cx);
  dbstmt->stmt = stmt;
  dbstmt->open = true;
  dbstmt->canGet = false;
  dbstmt->assoc_db = dbobj;
  dbstmt->current_row = JS::NullValue();
  dbobj->stmts.insert(dbstmt);

  JSObject* row = BuildObject(cx, &sqlite_stmt, sqlite_stmt_methods, sqlite_stmt_props, dbstmt);
  if (!row) {
    sqlite3_finalize(stmt);
    delete dbstmt;
    JS_free(cx, sql);
    THROW_ERROR(cx, "Could not create the sqlite row object");
  }
  JS_free(cx, sql);

  args.rval().setObjectOrNull(row);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_close) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SqliteDB* dbobj = (SqliteDB*)JS_GetInstancePrivate(cx, self, &sqlite_db, NULL);
  if (!clean_sqlite_db(dbobj)) {
    char msg[1024];
    sprintf_s(msg, sizeof(msg), "Could not close database: %s", sqlite3_errmsg(dbobj->db));
    THROW_ERROR(cx, msg);
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_open) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  SqliteDB* dbobj = (SqliteDB*)JS_GetInstancePrivate(cx, self, &sqlite_db, NULL);
  if (!dbobj->open) {
    if (SQLITE_OK != sqlite3_open16(dbobj->path.c_str(), &dbobj->db)) {
      char msg[1024];
      sprintf_s(msg, sizeof(msg), "Could not open database: %s", sqlite3_errmsg(dbobj->db));
      THROW_ERROR(cx, msg);
    }
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_PROP(sqlite_getProperty) {
  SqliteDB* dbobj = (SqliteDB*)JS_GetInstancePrivate(cx, obj, &sqlite_db, NULL);

  jsval ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case SQLITE_PATH:
      vp.setString(JS_NewUCStringCopyZ(cx, dbobj->path.c_str()));
      break;
    case SQLITE_OPEN:
      vp.setBoolean(dbobj->open);
      break;
    case SQLITE_LASTROWID:
      vp.setInt32(static_cast<int32_t>(sqlite3_last_insert_rowid(dbobj->db)));
      break;
    case SQLITE_STMTS: {
      JS_BeginRequest(cx);
      JSObject* stmts = JS_NewArrayObject(cx, dbobj->stmts.size(), NULL);
      vp.set(OBJECT_TO_JSVAL(stmts));
      int i = 0;
      for (StmtList::iterator it = dbobj->stmts.begin(); it != dbobj->stmts.end(); it++, i++) {
        if ((*it)->open) {
          JSObject* stmt = BuildObject(cx, &sqlite_stmt, sqlite_stmt_methods, sqlite_stmt_props, *it);
          jsval tmp = OBJECT_TO_JSVAL(stmt);
          JS_SetElement(cx, stmts, i, &tmp);
        }
      }
      JS_EndRequest(cx);
    } break;
    case SQLITE_CHANGES:
      vp.setDouble(sqlite3_changes(dbobj->db));
      // JS_NewNumberValue(cx, (jsdouble)sqlite3_changes(dbobj->db), vp);
      break;
  }
  return JS_TRUE;
}

void sqlite_finalize(JSFreeOp*, JSObject* obj) {
  SqliteDB* dbobj = (SqliteDB*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  if (dbobj) {
    clean_sqlite_db(dbobj);
    delete dbobj;
  }
}

// JSBool sqlite_equal(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
//{
//	SqliteDB* dbobj = (SqliteDB*)JS_GetInstancePrivate(cx, obj, &sqlite_db, NULL);
//	if(!JSVAL_IS_OBJECT(v))
//		return JS_TRUE;
//	JSObject *obj2 = JSVAL_TO_OBJECT(v);
//	if(!obj2 || JS_GET_CLASS(cx, obj2) != JS_GET_CLASS(cx, obj))
//		return JS_TRUE;
//
//	SqliteDB* dbobj2 = (SqliteDB*)JS_GetPrivate(obj2);
//	if(dbobj2->db != dbobj->db)
//		return JS_TRUE;
//
//	*bp = JS_TRUE;
//	return JS_TRUE;
//}

JSAPI_FUNC(sqlite_stmt_getobject) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  sqlite3_stmt* stmt = stmtobj->stmt;

  if (!stmtobj->canGet) {
    args.rval().setNull();
    return JS_TRUE;
  }
  if (!stmtobj->current_row.isNullOrUndefined()) {
    args.rval().set(stmtobj->current_row);
    return JS_TRUE;
  }

  int cols = sqlite3_column_count(stmt);
  if (cols == 0) {
    args.rval().setBoolean(true);
    return JS_TRUE;
  }
  JSObject* obj2 = JS_New(cx, NULL, NULL, NULL);
  // JSObject *obj2 = JS_ConstructObject(cx, NULL, NULL);
  jsval val;
  if (!obj2) THROW_ERROR(cx, "Failed to create row result object");
  for (int i = 0; i < cols; i++) {
    const char* colnam = sqlite3_column_name(stmt, i);
    switch (sqlite3_column_type(stmt, i)) {
      case SQLITE_INTEGER:
        // jsdouble == double, so this conversion is no problem
        val = JS_NumberValue((jsdouble)sqlite3_column_int64(stmt, i));
        if (!JS_SetProperty(cx, obj2, colnam, &val)) THROW_ERROR(cx, "Failed to add column to row results");
        break;
      case SQLITE_FLOAT:
        val = JS_NumberValue(sqlite3_column_double(stmt, i));
        if (!JS_SetProperty(cx, obj2, colnam, &val)) THROW_ERROR(cx, "Failed to add column to row results");
        break;
      case SQLITE_TEXT: {
        val = JS::StringValue(JS_NewStringCopyZ(cx, reinterpret_cast<const char*>(sqlite3_column_text(stmt, i))));
        if (!JS_SetProperty(cx, obj2, colnam, &val)) THROW_ERROR(cx, "Failed to add column to row results");
        break;
      }
      case SQLITE_BLOB:
        // currently not supported
        THROW_ERROR(cx, "Blob type not supported (yet)");
        break;
      case SQLITE_NULL:
        jsval nul = JS::NullValue();
        if (!JS_SetProperty(cx, obj2, colnam, &nul)) THROW_ERROR(cx, "Failed to add column to row results");
        break;
    }
  }

  stmtobj->current_row.setObjectOrNull(obj2);
  args.rval().setObjectOrNull(obj2);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_colcount) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  sqlite3_stmt* stmt = stmtobj->stmt;

  if (!stmtobj->canGet) THROW_ERROR(cx, "Statement is not ready");

  args.rval().setInt32(sqlite3_column_count(stmt));
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_colval) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  sqlite3_stmt* stmt = stmtobj->stmt;

  if (!stmtobj->canGet) THROW_ERROR(cx, "Statement is not ready");

  if (args.length() < 1 || args.length() > 1 || !args[0].isInt32())
    THROW_ERROR(cx, "Invalid parameter for SQLiteStatement.getColumnValue");

  int i = args[0].toInt32();
  switch (sqlite3_column_type(stmt, i)) {
    case SQLITE_INTEGER:
      // jsdouble == double, so this conversion is no problem
      args.rval().setNumber(static_cast<double>(sqlite3_column_int64(stmt, i)));
      break;
    case SQLITE_FLOAT:
      args.rval().setNumber(sqlite3_column_double(stmt, i));
      break;
    case SQLITE_TEXT: {
      args.rval().setString(JS_NewStringCopyZ(cx, reinterpret_cast<const char*>(sqlite3_column_text(stmt, i))));
      break;
    }
    case SQLITE_BLOB:
      // currently not supported
      THROW_ERROR(cx, "Blob type not supported (yet)");
      break;
    case SQLITE_NULL:
      args.rval().setNull();
      break;
  }
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_colname) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  sqlite3_stmt* stmt = stmtobj->stmt;

  if (!stmtobj->canGet) THROW_ERROR(cx, "Statement is not ready");

  if (args.length() < 1 || args.length() > 1 || !args[0].isInt32())
    THROW_ERROR(cx, "Invalid parameter for SQLiteStatement.getColumnValue");

  int i = args[0].toInt32();
  args.rval().setString(JS_NewStringCopyZ(cx, sqlite3_column_name(stmt, i)));
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_execute) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);

  int res = sqlite3_step(stmtobj->stmt);

  if (SQLITE_ROW != res && SQLITE_DONE != res) THROW_ERROR(cx, sqlite3_errmsg(stmtobj->assoc_db->db));
  close_db_stmt(stmtobj);

  args.rval().setBoolean(SQLITE_DONE == res);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_bind) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  sqlite3_stmt* stmt = stmtobj->stmt;
  if (args.length() < 2 || args.length() > 2 || !(args[0].isString() || args[0].isInt32()))
    THROW_ERROR(cx, "Invalid parameters for SQLiteStatement.bind");

  int colnum = -1;
  if (args[0].isInt32())
    colnum = args[0].toInt32();
  else {
    char* szText = JS_EncodeStringToUTF8(cx, args[0].toString());
    colnum = sqlite3_bind_parameter_index(stmt, szText);
    JS_free(cx, szText);
  }

  if (colnum == 0) THROW_ERROR(cx, "Invalid parameter number, parameters start at 1");

  char* szText;
  switch (JS_TypeOfValue(cx, args[1])) {
    case JSTYPE_VOID:
      sqlite3_bind_null(stmt, colnum);
      break;
    case JSTYPE_STRING:
      szText = JS_EncodeStringToUTF8(cx, args[1].toString());
      sqlite3_bind_text(stmt, colnum, szText, -1, SQLITE_STATIC);
      JS_free(cx, szText);
      break;
    case JSTYPE_NUMBER:
      if (args[1].isNumber())
        sqlite3_bind_double(stmt, colnum, args[1].toNumber());
      else if (args[1].isInt32())
        sqlite3_bind_int(stmt, colnum, args[1].toInt32());
      break;
    case JSTYPE_BOOLEAN:
      sqlite3_bind_text(stmt, colnum, args[1].toBoolean() ? "true" : "false", -1, SQLITE_STATIC);
      break;
    default:
      THROW_ERROR(cx, "Invalid bound parameter");
      break;
  }

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_next) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);

  int res = sqlite3_step(stmtobj->stmt);

  if (SQLITE_ROW != res && SQLITE_DONE != res) THROW_ERROR(cx, sqlite3_errmsg(stmtobj->assoc_db->db));

  stmtobj->canGet = !!(SQLITE_ROW == res);
  if (!stmtobj->current_row.isNullOrUndefined()) {
    stmtobj->current_row = JS::NullValue();
  }

  args.rval().setBoolean(SQLITE_ROW == res);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_skip) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  if (args.length() < 1 || !args[0].isInt32()) THROW_ERROR(cx, "Invalid parameter to SQLiteStatement.skip");
  for (int i = args[0].toInt32() - 1; i >= 0; i++) {
    int res = sqlite3_step(stmtobj->stmt);
    if (res != SQLITE_ROW) {
      if (res == SQLITE_DONE) {
        args.rval().setInt32((args[0].toInt32() - 1) - i);
        stmtobj->canGet = false;
        i = 0;
        continue;
      }
      THROW_ERROR(cx, sqlite3_errmsg(stmtobj->assoc_db->db));
    }
    stmtobj->canGet = true;
  }

  args.rval().set(args[0]);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_reset) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  if (SQLITE_OK != sqlite3_reset(stmtobj->stmt)) THROW_ERROR(cx, sqlite3_errmsg(stmtobj->assoc_db->db));
  stmtobj->canGet = false;

  args.rval().setBoolean(true);
  return JS_TRUE;
}

JSAPI_FUNC(sqlite_stmt_close) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv().toObjectOrNull();
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, self, &sqlite_stmt, NULL);
  close_db_stmt(stmtobj);
  delete stmtobj;

  JS_SetPrivate(self, NULL);
  args.rval().setBoolean(true);
  //	JS_ClearScope(cx, obj);
  if (JS_ValueToObject(cx, JSVAL_NULL, &self) == JS_FALSE) return JS_TRUE;

  return JS_TRUE;
}

JSAPI_PROP(sqlite_stmt_getProperty) {
  DBStmt* stmtobj = (DBStmt*)JS_GetInstancePrivate(cx, obj, &sqlite_stmt, NULL);

  jsval ID;
  JS_IdToValue(cx, id, &ID);
  switch (JSVAL_TO_INT(ID)) {
    case SQLITE_STMT_SQL: {
      wchar_t* wText = AnsiToUnicode(sqlite3_sql(stmtobj->stmt));
      vp.setString(JS_NewUCStringCopyZ(cx, wText));
      delete[] wText;
      break;
    }
    case SQLITE_STMT_READY:
      vp.setBoolean(stmtobj->canGet);
      break;
  }
  return JS_TRUE;
}

void sqlite_stmt_finalize(JSFreeOp*, JSObject* obj) {
  DBStmt* stmtobj = (DBStmt*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  if (stmtobj) {
    if (stmtobj->stmt && stmtobj->open) {
      stmtobj->assoc_db->stmts.erase(stmtobj);
      // if(stmtobj->current_row)
      //	JS_RemoveRoot( &stmtobj->current_row);
      close_db_stmt(stmtobj);
    }
    delete stmtobj;
  }
}
