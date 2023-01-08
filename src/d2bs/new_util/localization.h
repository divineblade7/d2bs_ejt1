#pragma once

#include <Windows.h>
#include <algorithm>
#include <cinttypes>
#include <string>

namespace d2bs {
namespace util {

inline std::string wide_to_codepage(const std::wstring& in, DWORD code_page) {
  std::string out;

  if (in.empty()) return out;

  // Calculate length of out string
  int32_t len =
      WideCharToMultiByte(code_page, 0, in.c_str(), static_cast<int32_t>(in.length()), 0, 0, nullptr, nullptr);
  out.resize(len);

  // Convert
  WideCharToMultiByte(code_page, 0, in.c_str(), static_cast<int32_t>(in.length()), out.data(), len, nullptr, nullptr);

  return out;
}

inline std::wstring codepage_to_wide(const std::string& in, DWORD code_page) {
  std::wstring out;

  if (in.empty()) return out;

  // Calculate length of out string
  int32_t len = MultiByteToWideChar(code_page, 0, in.c_str(), static_cast<int32_t>(in.length()), 0, 0);
  out.resize(len);

  // Convert
  MultiByteToWideChar(code_page, 0, in.c_str(), static_cast<int32_t>(in.length()), out.data(), len);

  return out;
}

inline std::string wide_to_ansi(const std::wstring& in) {
  return wide_to_codepage(in, CP_ACP);
}

inline std::wstring ansi_to_wide(const std::string& in) {
  return codepage_to_wide(in, CP_ACP);
}

inline std::string wide_to_utf8(const std::wstring& str) {
  return wide_to_codepage(str, CP_UTF8);
}

inline std::wstring utf8_to_wide(const std::string& str) {
  return codepage_to_wide(str, CP_UTF8);
}

inline std::string to_lower(const std::string& str) {
  std::string out = str;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<unsigned char>(tolower(c)); });
  return out;
}

inline std::wstring to_lower(const std::wstring& str) {
  std::wstring out = str;
  std::transform(out.begin(), out.end(), out.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
  return out;
}

inline std::string to_upper(const std::string& str) {
  std::string out = str;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<unsigned char>(toupper(c)); });
  return out;
}

inline std::wstring to_upper(const std::wstring& str) {
  std::wstring out = str;
  std::transform(out.begin(), out.end(), out.begin(), [](wchar_t c) { return static_cast<wchar_t>(towupper(c)); });
  return out;
}

inline bool to_bool(const char* str) {
  switch (tolower(str[0])) {
    case 't':
    case '1':
      return true;
    case 'f':
    case '0':
    default:
      return false;
  }
}

inline bool to_bool(const wchar_t* str) {
  switch (tolower(str[0])) {
    case 't':
    case '1':
      return true;
    case 'f':
    case '0':
    default:
      return false;
  }
}

}  // namespace util
}  // namespace d2bs
