//
// Created by Arseny Tolmachev on 2017/02/28.
//

#ifndef JUMANPP_CORE_TYPES_H
#define JUMANPP_CORE_TYPES_H

#include "util/types.hpp"

namespace jumanpp {
namespace core {

/**
 * Encodes pointer to entry
 *
 * Positive values are dictionary entry pointers
 * Negative values are special entry pointers (1-complement representation)
 */
enum class EntryPtr : i32 {};

inline bool isSpecial(EntryPtr ptr) { return static_cast<i32>(ptr) < 0; }

inline i32 idxFromEntryPtr(EntryPtr ptr) {
  i32 intValue = static_cast<i32>(ptr);
  if (intValue < 0) {
    return ~intValue;
  }
  return intValue;
}

}  // core
}  // jumanpp

#endif  // JUMANPP_CORE_TYPES_H
