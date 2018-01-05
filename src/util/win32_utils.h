#ifndef JUMANPP_WIN32_UTILS_H
#define JUMANPP_WIN32_UTILS_H

#ifdef _MSC_VER

#include <string>

#include "string_piece.h"

namespace jumanpp {
namespace util {

std::wstring to_wide_string(const StringPiece str);

}  // namespace util
}  // namespace jumanpp

#endif // _MSC_VER

#endif  // JUMANPP_WIN32_UTILS_H
