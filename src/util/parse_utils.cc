//
// Created by Arseny Tolmachev on 2018/05/16.
//

#include "parse_utils.h"

namespace jumanpp {
namespace util {

bool parseU64(StringPiece sp, u64* result) {
  if (sp.empty()) {
    return false;
  }

  u64 val = 0;
  for (const char* c = sp.char_begin(); c < sp.char_end(); ++c) {
    char x = *c;
    if (x >= '0' && x <= '9') {
      val = (val * 10) + (x - '0');
    } else {
      return false;
    }
  }
  *result = val;
  return true;
}

}  // namespace util
}  // namespace jumanpp