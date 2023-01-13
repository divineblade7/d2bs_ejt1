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

#define _USE_32BIT_TIME_T

#include "d2bs/script/api/JSFile.h"

#include "d2bs/core/File.h"
#include "d2bs/utils/Helpers.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <io.h>

struct FileData {
  int mode;
  wchar_t* path;
  bool autoflush, locked;
  FILE* fptr;
#if DEBUG
  const char* lockLocation;
  int line;
#endif
};

EMPTY_CTOR(file)

void file_finalize(JSFreeOp*, JSObject* obj) {
  FileData* fdata = (FileData*)JS_GetPrivate(obj);
  if (fdata) {
    free(fdata->path);
    if (fdata->fptr) {
      if (fdata->locked) {
        _unlock_file(fdata->fptr);
#if DEBUG
        fdata->lockLocation = __FILE__;
        fdata->line = __LINE__;
#endif
        fclose(fdata->fptr);
      } else
        _fclose_nolock(fdata->fptr);
    }
    delete fdata;
  }
}

JSAPI_PROP(file_readable) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setBoolean((fdata->fptr && !feof(fdata->fptr) && !ferror(fdata->fptr)));
  return JS_TRUE;
}

JSAPI_PROP(file_writeable) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setBoolean((fdata->fptr && !ferror(fdata->fptr) && (fdata->mode % 3) > FILE_READ));
  return JS_TRUE;
}

JSAPI_PROP(file_seekable) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setBoolean((fdata->fptr && !ferror(fdata->fptr)));
  return JS_TRUE;
}

JSAPI_PROP(file_mode) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setInt32((fdata->mode % 3));
  return JS_TRUE;
}

JSAPI_PROP(file_binaryMode) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setBoolean((fdata->mode > 2));
  return JS_TRUE;
}

JSAPI_PROP(file_length) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (fdata->fptr)
    vp.setInt32(_filelength(_fileno(fdata->fptr)));
  else
    vp.setInt32(0);  //= JSVAL_ZERO;
  return JS_TRUE;
}

JSAPI_PROP(file_path) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setString(JS_NewUCStringCopyZ(cx, fdata->path + 1));
  return JS_TRUE;
}

JSAPI_PROP(file_position) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (fdata->fptr) {
    if (fdata->locked)
      vp.setInt32(ftell(fdata->fptr));
    else
      vp.setInt32(_ftell_nolock(fdata->fptr));
  } else
    vp.setInt32(0);  //= JSVAL_ZERO;
  return JS_TRUE;
}

JSAPI_PROP(file_eof) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (fdata->fptr)
    vp.setBoolean(!!feof(fdata->fptr));
  else
    vp.set(JSVAL_TRUE);
  return JS_TRUE;
}

JSAPI_PROP(file_accessed) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  struct _stat filestat = {0};
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (fdata->fptr) {
    _fstat(_fileno(fdata->fptr), &filestat);
    vp.setNumber((double)filestat.st_atime);
  } else
    vp.setInt32(0);  //= JSVAL_ZERO;
  return JS_TRUE;
}

JSAPI_PROP(file_created) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  struct _stat filestat = {0};
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (fdata->fptr) {
    _fstat(_fileno(fdata->fptr), &filestat);
    vp.setDouble((double)filestat.st_ctime);
  } else
    vp.setInt32(0);
  return JS_TRUE;
}

JSAPI_PROP(file_modified) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  struct _stat filestat = {0};
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (fdata->fptr) {
    _fstat(_fileno(fdata->fptr), &filestat);
    vp.setNumber((double)filestat.st_mtime);
  } else
    vp.setInt32(0);
  return JS_TRUE;
}

JSAPI_PROP(file_autoflush) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  vp.setBoolean(fdata->autoflush);
  return JS_TRUE;
}

JSAPI_STRICT_PROP(file_autoflush_setter) {
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, obj, &file_class, NULL);
  if (!fdata) {
    THROW_ERROR(cx, "Couldn't get file object");
  }

  if (vp.isBoolean()) fdata->autoflush = !!vp.toBoolean();
  return JS_TRUE;
}

JSAPI_FUNC(file_open) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() < 2) THROW_ERROR(cx, "Not enough parameters, 2 or more expected");
  if (!args[0].isString()) THROW_ERROR(cx, "Parameter 1 not a string");
  if (!args[1].isInt32()) THROW_ERROR(cx, "Parameter 2 not a mode");

  // Convert from JS params to C values
  const wchar_t* file = JS_GetStringCharsZ(cx, args[0].toString());
  int32_t mode = args[1].toInt32();
  bool binary = false;
  bool autoflush = false;
  bool lockFile = false;
  if (args.length() > 2 && args[2].isBoolean()) binary = args[2].toBoolean();
  if (args.length() > 3 && args[3].isBoolean()) autoflush = args[3].toBoolean();
  if (args.length() > 4 && args[4].isBoolean()) lockFile = args[4].toBoolean();

  // Check that the path looks basically ok, validation is handled later
  if (file == NULL || file[0] == '\0') THROW_ERROR(cx, "Invalid file name");

  // this could be simplified to: mode > FILE_APPEND || mode < FILE_READ
  // but then it takes a hit for readability
  switch (mode) {
    // Good modes
    case FILE_READ:
    case FILE_WRITE:
    case FILE_APPEND:
      break;
    // Bad modes
    default:
      THROW_ERROR(cx, "Invalid file mode");
  }

  if (binary) mode += 3;

  static const wchar_t* modes[] = {L"rt", L"w+t", L"a+t", L"rb", L"w+b", L"a+b"};
  FILE* fptr = fileOpenRelScript(file, modes[mode], cx);

  // If fileOpenRelScript failed, it already reported the error
  if (fptr == NULL) return JS_FALSE;

  FileData* fdata = new FileData;
  if (!fdata) {
    fclose(fptr);
    THROW_ERROR(cx, "Couldn't allocate memory for the FileData object");
  }

  fdata->mode = mode;
  fdata->path = _wcsdup(file);
  fdata->autoflush = autoflush;
  fdata->locked = lockFile;
  fdata->fptr = fptr;
  if (lockFile) {
    _lock_file(fptr);
#if DEBUG
    fdata->lockLocation = __FILE__;
    fdata->line = __LINE__;
#endif
  }

  JSObject* res = BuildObject(cx, &file_class, file_methods, file_props, fdata);
  if (!res) {
    if (lockFile)
      fclose(fptr);
    else
      _fclose_nolock(fptr);
    free(fdata->path);
    delete fdata;
    THROW_ERROR(cx, "Failed to define the file object");
  }

  args.rval().setObjectOrNull(res);
  return JS_TRUE;
}

JSAPI_FUNC(file_close) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata) {
    if (fdata->fptr) {
      if (fdata->locked) {
        _unlock_file(fdata->fptr);
#if DEBUG
        fdata->lockLocation = __FILE__;
        fdata->line = __LINE__;
#endif
        if (!!fclose(fdata->fptr)) THROW_ERROR(cx, "Close failed");
      } else if (!!_fclose_nolock(fdata->fptr))
        THROW_ERROR(cx, "Close failed");
      fdata->fptr = NULL;
    } else
      THROW_ERROR(cx, "File is not open");
  }

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}

JSAPI_FUNC(file_reopen) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata) {
    if (!fdata->fptr) {
      static const wchar_t* modes[] = {L"rt", L"w+t", L"a+t", L"rb", L"w+b", L"a+b"};
      fdata->fptr = fileOpenRelScript(fdata->path, modes[fdata->mode], cx);

      // If fileOpenRelScript failed, it already reported the error
      if (fdata->fptr == NULL) return JS_FALSE;

      if (fdata->locked) {
        _lock_file(fdata->fptr);
#if DEBUG
        fdata->lockLocation = __FILE__;
        fdata->line = __LINE__;
#endif
      }
    } else {
      THROW_ERROR(cx, "File is not closed");
    }
  }

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}

JSAPI_FUNC(file_read) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    clearerr(fdata->fptr);
    int32_t count = 1;
    if (!(args.length() > 0 && args[0].toInt32() > 0 && JS_ValueToInt32(cx, args[0], &count)))
      THROW_ERROR(cx, "Invalid arguments");

    if (fdata->mode > 2) {
      // binary mode
      int* result = new int[count + 1];
      memset(result, 0, count + 1);
      uint32_t size = 0;
      if (fdata->locked)
        size = fread(result, sizeof(int), count, fdata->fptr);
      else
        size = _fread_nolock(result, sizeof(int), count, fdata->fptr);

      if (size != (uint32_t)count && ferror(fdata->fptr)) {
        delete[] result;
        THROW_ERROR(cx, "Read failed");
      }
      if (count == 1) {
        args.rval().setInt32(result[0]);
      } else {
        JSAutoRequest r(cx);
        JSObject* arr = JS_NewArrayObject(cx, 0, NULL);
        args.rval().setObjectOrNull(arr);
        for (int i = 0; i < count; i++) {
          JS::Value val = INT_TO_JSVAL(result[i]);
          JS_SetElement(cx, arr, i, &val);
        }
      }
    } else {
      uint32_t size = 0;
      int offset = 0;
      bool begin = false;

      if (fdata->locked)
        size = ftell(fdata->fptr);
      else
        size = _ftell_nolock(fdata->fptr);

      if (size == 0) begin = true;
      // text mode
      if (fdata->locked)
        fflush(fdata->fptr);
      else
        _fflush_nolock(fdata->fptr);

      char* result = new char[count + 1];
      memset(result, 0, count + 1);
      size = 0;
      if (fdata->locked)
        size = fread(result, sizeof(char), count, fdata->fptr);
      else
        size = _fread_nolock(result, sizeof(char), count, fdata->fptr);

      if (size != (uint32_t)count && ferror(fdata->fptr)) {
        delete[] result;
        THROW_ERROR(cx, "Read failed");
      }

      if (begin && size > 2 && is_bom(result)) {  // skip BOM
        offset = 3;
      }
      args.rval().setString(JS_NewStringCopyZ(cx, result + offset));
      delete[] result;
    }
  }

  return JS_TRUE;
}

JSAPI_FUNC(file_readLine) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    uint32_t size = 0;
    int offset = 0;
    bool begin = false;

    if (fdata->locked)
      size = ftell(fdata->fptr);
    else
      size = _ftell_nolock(fdata->fptr);

    if (size == 0) begin = true;

    char* line = readLine(fdata->fptr, fdata->locked);
    if (!line) THROW_ERROR(cx, "Read failed");

    if (begin && strlen(line) > 2 && is_bom(line)) {  // skip BOM
      offset = 3;
    }

    args.rval().setString(JS_NewStringCopyZ(cx, line + offset));
    free(line);
  }
  return JS_TRUE;
}

JSAPI_FUNC(file_readAllLines) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    JSObject* arr = JS_NewArrayObject(cx, 0, NULL);
    args.rval().setObjectOrNull(arr);
    int i = 0;
    JSAutoRequest r(cx);
    while (!feof(fdata->fptr)) {
      uint32_t size = 0;
      int offset = 0;
      bool begin = false;

      if (fdata->locked)
        size = ftell(fdata->fptr);
      else
        size = _ftell_nolock(fdata->fptr);

      if (size == 0) begin = true;

      char* line = readLine(fdata->fptr, fdata->locked);
      if (!line) THROW_ERROR(cx, "Read failed");

      if (begin && strlen(line) > 2 && is_bom(line)) {  // skip BOM
        offset = 3;
      }

      JS::Value val = JS::StringValue(JS_NewStringCopyZ(cx, line + offset));
      JS_SetElement(cx, arr, i++, &val);
      free(line);
    }
  }

  return JS_TRUE;
}

JSAPI_FUNC(file_readAll) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    uint32_t size = 0;
    int offset = 0;
    bool begin = false;

    if (fdata->locked)
      size = ftell(fdata->fptr);
    else
      size = _ftell_nolock(fdata->fptr);

    if (size == 0) {
      begin = true;
    }

    if (fdata->locked)
      fseek(fdata->fptr, 0, SEEK_END);
    else
      _fseek_nolock(fdata->fptr, 0, SEEK_END);

    size = 0;
    if (fdata->locked)
      size = ftell(fdata->fptr);
    else
      size = _ftell_nolock(fdata->fptr);

    if (fdata->locked)
      fseek(fdata->fptr, 0, SEEK_SET);
    else
      _fseek_nolock(fdata->fptr, 0, SEEK_SET);

    char* contents = new char[size + 1];
    memset(contents, 0, size + 1);
    uint32_t count = 0;
    if (fdata->locked)
      count = fread(contents, sizeof(char), size, fdata->fptr);
    else
      count = _fread_nolock(contents, sizeof(char), size, fdata->fptr);

    if (count != size && ferror(fdata->fptr)) {
      delete[] contents;
      THROW_ERROR(cx, "Read failed");
    }

    JSAutoRequest r(cx);
    if (begin && count > 2 && is_bom(contents)) {  // skip BOM
      offset = 3;
    }

    args.rval().setString(JS_NewStringCopyZ(cx, contents + offset));
    delete[] contents;
  }
  return JS_TRUE;
}

JSAPI_FUNC(file_write) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    for (uint32_t i = 0; i < argc; i++)
      writeValue(fdata->fptr, cx, JS_ARGV(cx, vp)[i], !!(fdata->mode > 2), fdata->locked);

    if (fdata->autoflush) {
      if (fdata->locked)
        fflush(fdata->fptr);
      else
        _fflush_nolock(fdata->fptr);
    }
  }

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}

JSAPI_FUNC(file_seek) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    if (args.length() > 0) {
      int32_t bytes;
      bool isLines = false, fromStart = false;
      if (JS_ValueToInt32(cx, args[0], &bytes) == JS_FALSE) THROW_ERROR(cx, "Could not convert parameter 1");
      if (args.length() > 1 && args[1].isBoolean()) isLines = args[1].toBoolean();
      if (args.length() > 2 && args[2].isBoolean()) fromStart = args[2].toBoolean();

      if (fromStart) rewind(fdata->fptr);

      if (!isLines) {
        if (fdata->locked && fseek(fdata->fptr, bytes, SEEK_CUR)) {
          THROW_ERROR(cx, "Seek failed");
        } else if (_fseek_nolock(fdata->fptr, bytes, SEEK_CUR))
          THROW_ERROR(cx, "Seek failed");
      } else {
        // semi-ugly hack to seek to the specified line
        // if I were unlazy I wouldn't be allocating/deallocating all this memory, but for now it's ok
        while (bytes--) free(readLine(fdata->fptr, fdata->locked));
      }
    } else
      THROW_ERROR(cx, "Not enough parameters");
  }

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}

JSAPI_FUNC(file_flush) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr)
    if (fdata->locked)
      fflush(fdata->fptr);
    else
      _fflush_nolock(fdata->fptr);

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}

JSAPI_FUNC(file_reset) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    if (fdata->locked && fseek(fdata->fptr, 0L, SEEK_SET)) {
      THROW_ERROR(cx, "Seek failed");
    } else if (_fseek_nolock(fdata->fptr, 0L, SEEK_SET))
      THROW_ERROR(cx, "Seek failed");
  }

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}

JSAPI_FUNC(file_end) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  auto self = args.thisv();
  FileData* fdata = (FileData*)JS_GetInstancePrivate(cx, self.toObjectOrNull(), &file_class, NULL);
  if (fdata && fdata->fptr) {
    if (fdata->locked && fseek(fdata->fptr, 0L, SEEK_END)) {
      THROW_ERROR(cx, "Seek failed");
    } else if (_fseek_nolock(fdata->fptr, 0L, SEEK_END))
      THROW_ERROR(cx, "Seek failed");
  }

  args.rval().setObjectOrNull(self.toObjectOrNull());
  return JS_TRUE;
}
