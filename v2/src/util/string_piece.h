//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_STRING_PIECE_H
#define JUMANPP_STRING_PIECE_H

#include <iosfwd>
#include <string>
#include "common.hpp"
#include "murmur_hash.h"
#include "types.hpp"

namespace jumanpp {

class StringPiece {
 public:
  using value_type = const char;
  using pointer_t = const value_type*;
  using iterator = pointer_t;

 private:
  pointer_t begin_;
  pointer_t end_;

 public:
  JPP_ALWAYS_INLINE constexpr StringPiece() noexcept : begin_{nullptr}, end_{nullptr} {}

  JPP_ALWAYS_INLINE constexpr StringPiece(iterator begin, iterator end) noexcept
      : begin_{begin}, end_{end} {}
  JPP_ALWAYS_INLINE constexpr StringPiece(pointer_t begin, size_t length) noexcept
      : begin_{begin}, end_{begin + length} {}
  JPP_ALWAYS_INLINE StringPiece(const std::string& str) noexcept
      : begin_{str.data()}, end_{str.data() + str.size()} {}

  JPP_ALWAYS_INLINE constexpr StringPiece(const StringPiece& other) noexcept = default;
  JPP_ALWAYS_INLINE StringPiece& operator=(const StringPiece& other) noexcept = default;

  /**
   * This constructor accepts only string literals.
   * It ignores null byte at the end if there is one.
   * @tparam array_size
   * @param array
   */
  template <size_t array_size>
  JPP_ALWAYS_INLINE constexpr StringPiece(const char (&array)[array_size]) noexcept
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
  JPP_ALWAYS_INLINE constexpr StringPiece(const Cont& cont) noexcept
      : begin_{cont.data()}, end_{cont.data() + cont.size()} {}

  JPP_ALWAYS_INLINE constexpr iterator begin() const noexcept { return begin_; }
  JPP_ALWAYS_INLINE constexpr iterator end() const noexcept { return end_; }
  JPP_ALWAYS_INLINE constexpr size_t size() const noexcept {
    return static_cast<size_t>(end_ - begin_);
  }

  std::string str() const { return std::string{begin_, end_}; }

  /**
   * Makes a slice (substring) of original string.
   * Slice is [from, to) in range notation.
   * If NDEBUG is not defined this function performs range checks.
   *
   * @param from index of first character in slice
   * @param to index after the last character in slice
   * @return new StringPiece with specified length
   */
  JPP_ALWAYS_INLINE constexpr StringPiece slice(ptrdiff_t from, ptrdiff_t to) const noexcept {
    JPP_DCHECK_GE(from, 0);
    JPP_DCHECK_LE(to, size());
    JPP_DCHECK_LE(from, to);
    return StringPiece{begin() + from, begin() + to};
  }
};

std::ostream& operator<<(std::ostream& str, const StringPiece& sp);

bool operator==(const StringPiece& l, const StringPiece& r);
inline bool operator!=(const StringPiece& l, const StringPiece& r) {
  return !(l == r);
}

namespace util {
namespace impl {
struct StringPieceHash {
  static const constexpr u64 StringPieceSeed = 0x4adf325;

  inline size_t operator()(const StringPiece& p) const noexcept {
    using ptrtype = const u8*;
    auto start = reinterpret_cast<ptrtype>(p.begin());
    auto end = reinterpret_cast<ptrtype>(p.end());
    auto hv =
        jumanpp::util::hashing::murmurhash3_memory(start, end, StringPieceSeed);
    return static_cast<size_t>(hv);
  }
};
}  // impl
}
}  // jumanpp

namespace std {
template <>
class hash<jumanpp::StringPiece> {
  jumanpp::util::impl::StringPieceHash impl_;

 public:
  using argument_type = jumanpp::StringPiece;
  using result_type = size_t;
  inline result_type operator()(argument_type const& p) const noexcept {
    return impl_(p);
  }
};
}  // std

#endif  // JUMANPP_STRING_PIECE_H
