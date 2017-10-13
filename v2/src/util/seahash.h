//
// Created by Arseny Tolmachev on 2017/05/29.
//

#ifndef JUMANPP_SEAHASH_H
#define JUMANPP_SEAHASH_H

#include "common.hpp"
#include "types.hpp"

namespace jumanpp {
namespace util {
namespace hashing {

static constexpr u64 SeaHashSeed0 = 0x16f11fe89b0d677cULL;
static constexpr u64 SeaHashSeed1 = 0xb480a793d8e6c86cULL;
static constexpr u64 SeaHashMult = 0x6eed0e9da4d94a4fULL;

// See http://ticki.github.io/blog/seahash-explained/ for explanation.
// This implementation uses only two u64 for states instead of four.

struct SeaHashState {
  u64 s0 = SeaHashSeed0;
  u64 s1 = SeaHashSeed1;

  static SeaHashState withSeed(u64 seed) {
    return SeaHashState{SeaHashSeed0 ^ diffuse(seed),
                        SeaHashSeed1 ^ diffuse(seed)};
  }

  JPP_ALWAYS_INLINE inline static u64 diffuse(u64 v) {
    v *= SeaHashMult;
    auto a = v >> 32;
    auto b = static_cast<unsigned char>(v >> 60);
    v ^= a >> b;
    v *= SeaHashMult;
    return v;
  }

  JPP_ALWAYS_INLINE inline SeaHashState mix(u64 v1, u64 v2) const noexcept {
    return SeaHashState{diffuse(s0 ^ v1), diffuse(s1 ^ v2)};
  }

  JPP_ALWAYS_INLINE inline SeaHashState mix(u64 v0) {
    return SeaHashState{s1, diffuse(v0 ^ s0)};
  }

  JPP_ALWAYS_INLINE inline u64 finish() const noexcept {
    return diffuse(s0 ^ s1);
  }
};

class SeaHashLite {
  u64 state_;

  constexpr static JPP_ALWAYS_INLINE inline u64 diffuse(u64 v) {
    v *= SeaHashMult;
    auto a = v >> 32;
    auto b = static_cast<unsigned char>(v >> 60);
    return v ^ (a >> b);
  }

 public:
  constexpr SeaHashLite() noexcept : state_{SeaHashSeed0} {}
  constexpr explicit SeaHashLite(u64 state) noexcept : state_{state} {}

  constexpr JPP_ALWAYS_INLINE SeaHashLite mix(u64 v) const noexcept {
    return SeaHashLite{diffuse(state_ ^ v)};
  }

  constexpr JPP_ALWAYS_INLINE u64 finish() const noexcept {
    return diffuse(state_ ^ SeaHashSeed1);
  }

  constexpr JPP_ALWAYS_INLINE u64 state() const noexcept { return state_; }
};

namespace detail {
JPP_ALWAYS_INLINE inline SeaHashLite seaHashSeqImpl(SeaHashLite h) { return h; }

JPP_ALWAYS_INLINE inline SeaHashLite seaHashSeqImpl(SeaHashLite h, u64 one) {
  return h.mix(one);
}

template <typename... Args>
JPP_ALWAYS_INLINE inline SeaHashLite seaHashSeqImpl(SeaHashLite h, u64 arg,
                                                    Args... args) {
  return seaHashSeqImpl(h.mix(arg), args...);
}
}  // namespace detail

/**
 * Hash sequence with compile-time passed parameters.
 * This version pushes tuple size as a first argument.
 *
 * Implementation uses C++11 vararg templates.
 * @param seed seed value for hash calcuation
 * @param args values for hash calculation
 * @return hash value
 */
template <typename... Args>
JPP_ALWAYS_INLINE inline u64 seaHashSeq(Args... args) {
  return detail::seaHashSeqImpl(SeaHashLite{}, sizeof...(args),
                                static_cast<u64>(args)...)
      .finish();
}

template <typename... Args>
JPP_ALWAYS_INLINE inline u64 rawSeahashStart(u64 count, Args... args) {
  return detail::seaHashSeqImpl(SeaHashLite{}, count, static_cast<u64>(args)...)
      .state();
}

template <typename... Args>
JPP_ALWAYS_INLINE inline u64 rawSeahashContinue(u64 state, Args... args) {
  return detail::seaHashSeqImpl(SeaHashLite{state}, static_cast<u64>(args)...)
      .state();
}

template <typename... Args>
JPP_ALWAYS_INLINE inline u64 rawSeahashFinish(u64 state, Args... args) {
  return detail::seaHashSeqImpl(SeaHashLite{state}, static_cast<u64>(args)...)
      .finish();
}

// This version pushes tuple size as a last argument.
template <typename... Args>
JPP_ALWAYS_INLINE inline u64 seaHashSeq2(Args... args) {
  return detail::seaHashSeqImpl(SeaHashLite{}, static_cast<u64>(args)...,
                                sizeof...(args))
      .finish();
}

template <typename Seq, typename Idx>
inline u64 seaHashIndexedSeq(u64 seed, const Seq& seq, const Idx& idx) {
  auto numIndices = idx.size();
  SeaHashLite state{numIndices};
  state = state.mix(seed);
  for (auto i : idx) {
    state = state.mix(seq[i]);
  }
  return state.finish();
}

}  // namespace hashing
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_SEAHASH_H
