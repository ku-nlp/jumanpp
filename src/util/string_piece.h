//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_STRING_PIECE_H
#define JUMANPP_STRING_PIECE_H

#include <algorithm>
#include <iosfwd>
#include <string>
#include "common.hpp"
#include "murmur_hash.h"
#include "types.hpp"

namespace jumanpp {

class StringPiece {
 public:
  using byte_t = const u8;
  using byte_ptr = byte_t*;

  using value_type = const char;
  using pointer_t = const value_type*;
  using iterator = pointer_t;

 private:
  pointer_t begin_;
  pointer_t end_;

 public:
  JPP_ALWAYS_INLINE constexpr StringPiece() noexcept
      : begin_{nullptr}, end_{nullptr} {}

  JPP_ALWAYS_INLINE constexpr StringPiece(pointer_t begin,
                                          pointer_t end) noexcept
      : begin_{begin}, end_{end} {
    JPP_DCHECK_LE(begin, end);
  }

  JPP_ALWAYS_INLINE constexpr StringPiece(pointer_t begin,
                                          size_t length) noexcept
      : begin_{begin}, end_{begin + length} {}

  JPP_ALWAYS_INLINE StringPiece(byte_ptr begin, byte_ptr end) noexcept
      : begin_{reinterpret_cast<pointer_t>(begin)},
        end_{reinterpret_cast<pointer_t>(end)} {};

  JPP_ALWAYS_INLINE StringPiece(byte_ptr begin, size_t length) noexcept
      : StringPiece(begin, begin + length) {}

  JPP_ALWAYS_INLINE constexpr StringPiece(const StringPiece& other) noexcept =
      default;

  JPP_ALWAYS_INLINE StringPiece& operator=(const StringPiece& other) noexcept =
      default;

  /**
   * This constructor accepts only string literals.
   * It ignores null byte at the end if there is one.
   * @tparam array_size
   * @param array
   */
  template <size_t array_size>
  JPP_ALWAYS_INLINE
  // MSVC has a bug with constexpr here :|
#ifndef _MSC_VER
      constexpr
#endif
      StringPiece(const char (&array)[array_size]) noexcept
      : begin_{array}, end_{array + array_size - 1} {
  }

  /**
   * Constructor for everything that has .data(), .size() and its type can be
   * assignable to const char
   * @tparam Cont
   * @param cont
   */
  template <typename Cont,
            typename Sz = typename std::enable_if<
                std::is_same<char, typename std::remove_cv<
                                       typename Cont::value_type>::type>::value,
                size_t>::type>
  JPP_ALWAYS_INLINE constexpr StringPiece(const Cont& cont) noexcept
      : begin_{cont.data()}, end_{cont.data() + static_cast<Sz>(cont.size())} {
    // static_cast here is to make MSVC compute the second template parameter
    // and SFINAE on bad things.
    // It won't SFINAE otherwise. Bad MSVC.
  }

  JPP_ALWAYS_INLINE constexpr pointer_t data() const noexcept { return begin_; }
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
  JPP_ALWAYS_INLINE constexpr StringPiece slice(ptrdiff_t from,
                                                ptrdiff_t to) const noexcept {
    JPP_DCHECK_GE(from, 0);
    JPP_DCHECK_LE(to, size());
    JPP_DCHECK_LE(from, to);
    auto begin_new = std::min(begin() + from, end());
    auto end_new = std::min(begin() + to, end());
    return StringPiece{begin_new, end_new};
  }

  JPP_ALWAYS_INLINE constexpr StringPiece from(ptrdiff_t from) const noexcept {
    JPP_DCHECK_IN(from, 0, size());
    return StringPiece{std::min(begin() + from, end()), end()};
  }

  JPP_ALWAYS_INLINE constexpr StringPiece take(ptrdiff_t num) const noexcept {
    JPP_DCHECK_IN(num, 0, size());
    return StringPiece{begin(), std::min(begin() + num, end())};
  }

  char operator[](size_t pos) const noexcept {
    JPP_DCHECK_IN(pos, 0, size());
    return begin_[pos];
  }

  bool empty() const noexcept { return begin_ == end_; }

  JPP_ALWAYS_INLINE constexpr pointer_t char_begin() const noexcept {
    return begin_;
  };

  JPP_ALWAYS_INLINE constexpr pointer_t char_end() const noexcept {
    return end_;
  };

  JPP_ALWAYS_INLINE byte_ptr ubegin() const noexcept {
    return reinterpret_cast<byte_ptr>(begin_);
  }

  JPP_ALWAYS_INLINE byte_ptr uend() const noexcept {
    return reinterpret_cast<byte_ptr>(end_);
  }

  static StringPiece fromCString(const char* data);

  template <typename C>
  void assignTo(C& result) {
    result.assign(begin(), size());
  }
};

static constexpr const StringPiece EMPTY_SP = StringPiece{};

std::ostream& operator<<(std::ostream& str, const StringPiece& sp);

bool operator==(const StringPiece& l, const StringPiece& r);
inline bool operator!=(const StringPiece& l, const StringPiece& r) {
  return !(l == r);
}
bool operator<(StringPiece s1, StringPiece s2);

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
}  // namespace impl
}  // namespace util
}  // namespace jumanpp

namespace std {
template <>
struct hash<jumanpp::StringPiece> {
  jumanpp::util::impl::StringPieceHash impl_;
  using argument_type = jumanpp::StringPiece;
  using result_type = size_t;
  inline result_type operator()(argument_type const& p) const noexcept {
    return impl_(p);
  }
};
}  // namespace std

#endif  // JUMANPP_STRING_PIECE_H
