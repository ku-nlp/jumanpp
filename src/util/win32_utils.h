#ifndef JUMANPP_WIN32_UTILS_H
#define JUMANPP_WIN32_UTILS_H

#ifdef _WIN32_WINNT

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <string>

#include "string_piece.h"

namespace jumanpp {
namespace util {

std::wstring to_wide_string(const StringPiece str);

}  // namespace util
}  // namespace jumanpp

#endif  // _WIN32_WINNT

#endif  // JUMANPP_WIN32_UTILS_H
