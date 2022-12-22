#include "d2bs/script/api/JSHash.h"

#include "d2bs/D2BS.h"
#include "d2bs/core/File.h"
#include "d2bs/core/Hash.h"
#include "d2bs/utils/Helpers.h"

JSAPI_FUNC(my_md5) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  char* input = JS_EncodeStringToUTF8(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  char* result = md5(input);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_InternString(cx, result)));
  delete[] result;
  JS_free(cx, input);
  return JS_TRUE;
}

JSAPI_FUNC(my_sha1) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  char* input = JS_EncodeStringToUTF8(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  char* result = sha1(input);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_InternString(cx, result)));
  delete[] result;
  JS_free(cx, input);
  return JS_TRUE;
}

JSAPI_FUNC(my_sha256) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  char* input = JS_EncodeStringToUTF8(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  char* result = sha256(input);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_InternString(cx, result)));
  delete[] result;
  JS_free(cx, input);
  return JS_TRUE;
}

JSAPI_FUNC(my_sha384) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  char* input = JS_EncodeStringToUTF8(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  char* result = sha384(input);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_InternString(cx, result)));
  delete[] result;
  JS_free(cx, input);
  return JS_TRUE;
}

JSAPI_FUNC(my_sha512) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  char* input = JS_EncodeStringToUTF8(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  char* result = sha512(input);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_InternString(cx, result)));
  delete[] result;
  JS_free(cx, input);
  return JS_TRUE;
}

JSAPI_FUNC(my_md5_file) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  const wchar_t* file = JS_GetStringCharsZ(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  if (!(file && file[0] && isValidPath(file))) THROW_ERROR(cx, "Invalid file path!");

  auto path = (Vars.script_dir / file).make_preferred().wstring();

  char* result = md5_file(&path[0]);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, result)));
  delete[] result;
  return JS_TRUE;
}

JSAPI_FUNC(my_sha1_file) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  const wchar_t* file = JS_GetStringCharsZ(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  if (!(file && file[0] && isValidPath(file))) THROW_ERROR(cx, "Invalid file path!");

  auto path = (Vars.script_dir / file).make_preferred().wstring();

  char* result = sha1_file(&path[0]);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, result)));
  delete[] result;
  return JS_TRUE;
}

JSAPI_FUNC(my_sha256_file) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  const wchar_t* file = JS_GetStringCharsZ(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  if (!(file && file[0] && isValidPath(file))) THROW_ERROR(cx, "Invalid file path!");

  auto path = (Vars.script_dir / file).make_preferred().wstring();

  char* result = sha256_file(&path[0]);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, result)));
  delete[] result;
  return JS_TRUE;
}

JSAPI_FUNC(my_sha384_file) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  const wchar_t* file = JS_GetStringCharsZ(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  if (!(file && file[0] && isValidPath(file))) THROW_ERROR(cx, "Invalid file path!");

  auto path = (Vars.script_dir / file).make_preferred().wstring();

  char* result = sha384_file(&path[0]);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, result)));
  delete[] result;
  return JS_TRUE;
}

JSAPI_FUNC(my_sha512_file) {
  if (argc != 1) THROW_ERROR(cx, "Invalid arguments");

  const wchar_t* file = JS_GetStringCharsZ(cx, JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  if (!(file && file[0] && isValidPath(file))) THROW_ERROR(cx, "Invalid file path!");

  auto path = (Vars.script_dir / file).make_preferred().wstring();

  char* result = sha512_file(&path[0]);
  if (result && result[0]) JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, result)));
  delete[] result;
  return JS_TRUE;
}
