//
// Created by Arseny Tolmachev on 2017/02/28.
//

#ifndef JUMANPP_CORE_TYPES_H
#define JUMANPP_CORE_TYPES_H

#include <limits>
#include "util/common.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {

/**
 * Encodes pointer to entry
 *
 * Positive values are dictionary entry pointers
 * Negative values are special entry pointers (1-complement representation)
 */
class EntryPtr {
  i32 value_;

 public:
  constexpr explicit EntryPtr(i32 value) : value_{value} {}
  constexpr EntryPtr(const EntryPtr&) = default;

  constexpr inline bool isSpecial() const { return value_ < 0; }

  constexpr inline bool isDic() const { return value_ >= 0; }

  inline i32 dicPtr() const {
    JPP_DCHECK_GE(value_, 0);
    return value_;
  }

  inline i32 extPtr() const {
    JPP_DCHECK_LT(value_, 0);
    return ~value_;
  }

  static constexpr EntryPtr BOS() {
    return EntryPtr{std::numeric_limits<i32>::min()};
  }

  static constexpr EntryPtr EOS() {
    return EntryPtr{std::numeric_limits<i32>::min() + 1};
  }

  inline i32 rawValue() const { return value_; }

  inline bool operator==(const EntryPtr& o) const { return value_ == o.value_; }
  inline bool operator!=(const EntryPtr& o) const { return value_ != o.value_; }
};

class NodeInfo {
  /**
   * Entry pointer
   */
  EntryPtr entry_;

  /**
   * Surface length in codepoints
   */
  i32 codepoints_;

 public:
  NodeInfo() : entry_{EntryPtr::EOS()}, codepoints_{0} {}
  NodeInfo(const EntryPtr& entry_, i32 codepoints_)
      : entry_(entry_), codepoints_(codepoints_) {}

  EntryPtr entryPtr() const { return entry_; }

  i32 numCodepoints() const { return codepoints_; }
};

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_CORE_TYPES_H
