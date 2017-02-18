//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_STRING_PIECE_H
#define JUMANPP_STRING_PIECE_H

#include <iosfwd>
#include <string>
#include "common.hpp"
#include "types.hpp"

namespace jumanpp {

class StringPiece {
  using value_type = const char;
  using pointer_t = const value_type*;
  using iterator = pointer_t;

  pointer_t begin_;
  pointer_t end_;

 public:
  constexpr StringPiece(iterator begin, iterator end) noexcept
      : begin_{begin}, end_{end} {}
  constexpr StringPiece(pointer_t begin, size_t length) noexcept
      : begin_{begin}, end_{begin + length} {}
  StringPiece(const std::string& str) noexcept
      : begin_{str.data()}, end_{str.data() + str.size()} {}

  /**
   * This constructor accepts only string literals.
   * It ignores null byte at the end if there is one.
   * @tparam array_size
   * @param array
   */
  template <size_t array_size>
  constexpr StringPiece(const char (&array)[array_size]) noexcept
      : begin_{array}, end_{array + array_size} {
    if (*(end_ - 1) == 0) {
      --end_;
    }
  }

  explicit StringPiece(pointer_t begin);

  /**
   * Constructor for everything that has .data(), .size() and its type can be
   * assignable to const char
   * @tparam Cont
   * @param cont
   */
  template <typename Cont,
            typename = std::enable_if<std::is_trivially_assignable<
                value_type, typename Cont::value_type>::value>>
  constexpr StringPiece(const Cont& cont) noexcept
      : begin_{cont.data()}, end_{cont.data() + cont.size()} {}

  constexpr iterator begin() const noexcept { return begin_; }
  constexpr iterator end() const noexcept { return end_; }
  constexpr size_t size() const noexcept {
    return static_cast<size_t>(end_ - begin_);
  }

  std::string str() const { return std::string{begin_, end_}; }
};

std::ostream& operator<<(std::ostream& str, const StringPiece& sp);

bool operator==(const StringPiece& l, const StringPiece& r);
inline bool operator!=(const StringPiece& l, const StringPiece& r) {
  return !(l == r);
}
}

#endif  // JUMANPP_STRING_PIECE_H
