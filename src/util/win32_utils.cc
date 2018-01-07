#include "win32_utils.h"

#define NOMINMAX
#include <windows.h>

namespace jumanpp {
namespace util {

std::wstring to_wide_string(const StringPiece str) {
  int str_size = static_cast<int>(str.size());
  int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size, NULL, 0);
  std::wstring result(size, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size, &result[0], size);
  return result;
}

}  // namespace util
}  // namespace jumanpp
