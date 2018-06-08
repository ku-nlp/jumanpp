//
// Created by Arseny Tolmachev on 2018/05/16.
//

#ifndef JUMANPP_PARSE_UTILS_H
#define JUMANPP_PARSE_UTILS_H

#include "string_piece.h"

namespace jumanpp {
namespace util {

bool parseU64(StringPiece sp, u64* result);

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_PARSE_UTILS_H
