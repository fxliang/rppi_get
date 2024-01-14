#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>

namespace encoding {

#ifdef _WIN32
inline std::string ansi2utf8(const std::string &str) {
  int wCharLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
  if (wCharLen == 0)
    return "";
  wchar_t *wStr = new wchar_t[wCharLen];
  MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wStr, wCharLen);
  int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, 0, 0, 0, 0);
  if (utf8Len == 0) {
    delete[] wStr;
    return "";
  }
  char *utf8Str = new char[utf8Len];
  WideCharToMultiByte(CP_UTF8, 0, wStr, -1, utf8Str, utf8Len, 0, 0);
  std::string result(utf8Str);
  delete[] wStr;
  delete[] utf8Str;
  return result;
}
#endif

inline std::string platform_str(const std::string &str) {
#ifdef _WIN32
  return ansi2utf8(str);
#else
  return str;
#endif
}

} // namespace encoding