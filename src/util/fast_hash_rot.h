//
// Created by Arseny Tolmachev on 2021/07/02.
//

#ifndef JUMANPP_FAST_HASH_ROT_H
#define JUMANPP_FAST_HASH_ROT_H

#include "util/seahash.h"
#include "util/types.hpp"

namespace jumanpp {
namespace util {
namespace hashing {

template <size_t s>
JPP_ALWAYS_INLINE inline u64 rotl(u64 v) {
  constexpr size_t rest = 64 - s;
  static_assert(rest <= 64, "rest must be > 0");
  // compilers are good at optimizing this stuff
  // this will be one instruction on sane platforms
  return (v << s) | (v >> rest);
}

/**
 * This is a modification of FastHash1 which uses ROT(ation) operation
 * instead of shift+xor combination.
 * This reduces hot stop dependency chain instruction count by one giving ~15%
 * speed increase without making hash function visibly worse.
 */
class FastHashRot {
  u64 state_;

 public:
  FastHashRot() noexcept : state_{::jumanpp::util::hashing::SeaHashSeed0} {}
  explicit FastHashRot(u64 state) noexcept : state_{state} {}
  explicit FastHashRot(const u64* const state) noexcept : state_{*state} {}

  JPP_ALWAYS_INLINE FastHashRot mix(u64 data) const noexcept {
    u64 v = state_ ^ data;
    v *= ::jumanpp::util::hashing::SeaHashMult;
    v = rotl<32>(v);
    return FastHashRot{v};
  }

  JPP_ALWAYS_INLINE FastHashRot mixMem(const u64* const data) const noexcept {
    return mix(*data);
  }

  JPP_ALWAYS_INLINE u64 result() const noexcept { return state_; }

  template <typename T>
  JPP_ALWAYS_INLINE T masked(T mask) const noexcept {
    return static_cast<T>(state_) & mask;
  }
};

}  // namespace hashing
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_FAST_HASH_ROT_H
