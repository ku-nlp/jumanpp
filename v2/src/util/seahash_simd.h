//
// Created by Arseny Tolmachev on 2017/10/16.
//

#ifndef JUMANPP_SEAHASH_SIMD_H
#define JUMANPP_SEAHASH_SIMD_H

#include "common.hpp"
#include "seahash.h"
#include "types.hpp"

#ifdef __AVX2__
#include <immintrin.h>
#define JPP_AVX2 1
#define JPP_WITH_AVX2(x) x
#else
#define JPP_WITH_AVX2(x)
#endif

#ifdef __SSE4_1__
#include <nmmintrin.h>
#include <xmmintrin.h>
#define JPP_SSE_IMPL
#endif

namespace jumanpp {
namespace util {
namespace hashing {

class FastHash1 {
  u64 state_;

 public:
  FastHash1() noexcept : state_{SeaHashSeed0} {}
  explicit FastHash1(u64 state) noexcept : state_{state} {}
  explicit FastHash1(const u64* state) noexcept : state_{*state} {}

  JPP_ALWAYS_INLINE FastHash1 mix(const u64* data) const noexcept {
    u64 v = state_ ^ *data;
    v *= SeaHashMult;
    v = v ^ (v >> 32);
    return FastHash1{v};
  }

  JPP_ALWAYS_INLINE u64 result() const noexcept { return state_; }

  template <typename T>
  JPP_ALWAYS_INLINE T masked(T mask) const noexcept {
    return static_cast<T>(state_) & mask;
  }
};

#define JPP_BITS8(a0, a1, a2, a3, a4, a5, a6, a7)                          \
  ((a0 << 0) | (a1 << 1) | (a2 << 2) | (a3 << 3) | (a4 << 4) | (a5 << 5) | \
   (a6 << 6) | (a7 << 7))

#ifdef JPP_SSE_IMPL

class FastHash2 {
  __m128i state_;

 public:
  FastHash2() noexcept : state_(_mm_set1_epi64x(SeaHashSeed0)) {}
  explicit FastHash2(const u64* state) noexcept
      : state_(_mm_loadu_si128(reinterpret_cast<const __m128i*>(state))) {}
  explicit FastHash2(__m128i state) noexcept : state_{state} {}

  JPP_ALWAYS_INLINE static __m128i Multiply64Bit(__m128i a, __m128i b) {
    auto ax0_ax1_ay0_ay1 = a;
    auto bx0_bx1_by0_by1 = b;

    // i means ignored

    auto ax1_i_ay1_i =
        _mm_shuffle_epi32(ax0_ax1_ay0_ay1, _MM_SHUFFLE(3, 3, 1, 1));
    auto bx1_i_by1_i =
        _mm_shuffle_epi32(bx0_bx1_by0_by1, _MM_SHUFFLE(3, 3, 1, 1));

    auto ax0bx0_ay0by0 = _mm_mul_epu32(ax0_ax1_ay0_ay1, bx0_bx1_by0_by1);
    auto ax0bx1_ay0by1 = _mm_mul_epu32(ax0_ax1_ay0_ay1, bx1_i_by1_i);
    auto ax1bx0_ay1by0 = _mm_mul_epu32(ax1_i_ay1_i, bx0_bx1_by0_by1);

    auto ax0bx1_ay0by1_32 = _mm_slli_epi64(ax0bx1_ay0by1, 32);
    auto ax1bx0_ay1by0_32 = _mm_slli_epi64(ax1bx0_ay1by0, 32);

    return _mm_add_epi64(ax0bx0_ay0by0,
                         _mm_add_epi64(ax0bx1_ay0by1_32, ax1bx0_ay1by0_32));
  }

  JPP_ALWAYS_INLINE FastHash2 mix(const u64* data) const noexcept {
    auto arg = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
    return mixImpl(arg);
  }

  JPP_ALWAYS_INLINE FastHash2 mixGather(const u64* data,
                                        const u32* indices) const noexcept {
    auto arg = _mm_undefined_si128();
    arg[0] = data[indices[0]];
    arg[1] = data[indices[1]];
    return mixImpl(arg);
  }

  JPP_ALWAYS_INLINE FastHash2 mixImpl(__m128i arg) const noexcept {
    auto v = _mm_xor_si128(state_, arg);
    v = Multiply64Bit(v, _mm_set1_epi64x(SeaHashMult));
    auto x = _mm_srli_epi64(v, 32);
    v = _mm_xor_si128(v, x);
    return FastHash2{v};
  }

  JPP_ALWAYS_INLINE void jointMaskStore(FastHash2 other, u32 mask,
                                        u32* result) {
    auto v1 = _mm_shuffle_epi32(state_, _MM_SHUFFLE(2, 0, 2, 0));
    auto v2 = _mm_shuffle_epi32(other.state_, _MM_SHUFFLE(0, 2, 0, 2));
    auto mask_reg = _mm_set1_epi32(mask);
    auto s = _mm_blend_epi16(v1, v2, JPP_BITS8(0, 0, 0, 0, 1, 1, 1, 1));
    s = _mm_and_si128(s, mask_reg);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(result), s);
  }

  void store(u64* addr) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(addr), state_);
  }
};

#endif

#ifdef JPP_AVX2

class FastHash4 {
  __m256i state_;

 public:
  FastHash4() noexcept : state_(_mm256_set1_epi64x(SeaHashSeed0)) {}
  explicit FastHash4(const u64* state) noexcept
      : state_(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(state))) {}
  explicit FastHash4(__m256i state) noexcept : state_{state} {}

  JPP_ALWAYS_INLINE static __m256i Multiply64Bit(__m256i a, __m256i b) {
    auto ax0_ax1_ay0_ay1 = a;
    auto bx0_bx1_by0_by1 = b;

    // i means ignored

    auto ax1_i_ay1_i =
        _mm256_shuffle_epi32(ax0_ax1_ay0_ay1, _MM_SHUFFLE(3, 3, 1, 1));
    auto bx1_i_by1_i =
        _mm256_shuffle_epi32(bx0_bx1_by0_by1, _MM_SHUFFLE(3, 3, 1, 1));

    auto ax0bx0_ay0by0 = _mm256_mul_epu32(ax0_ax1_ay0_ay1, bx0_bx1_by0_by1);
    auto ax0bx1_ay0by1 = _mm256_mul_epu32(ax0_ax1_ay0_ay1, bx1_i_by1_i);
    auto ax1bx0_ay1by0 = _mm256_mul_epu32(ax1_i_ay1_i, bx0_bx1_by0_by1);

    auto ax0bx1_ay0by1_32 = _mm256_slli_epi64(ax0bx1_ay0by1, 32);
    auto ax1bx0_ay1by0_32 = _mm256_slli_epi64(ax1bx0_ay1by0, 32);

    return _mm256_add_epi64(
        ax0bx0_ay0by0, _mm256_add_epi64(ax0bx1_ay0by1_32, ax1bx0_ay1by0_32));
  }

  JPP_ALWAYS_INLINE FastHash4 mix(const u64* data) const noexcept {
    auto arg = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data));
    return mixImpl(arg);
  }

  JPP_ALWAYS_INLINE FastHash4 mixGatherInstr(const u64* base,
                                             const u32* indices) const
      noexcept {
    auto idx = _mm_loadu_si128(reinterpret_cast<const __m128i*>(indices));
    auto ptr = reinterpret_cast<const long long int*>(base);
    auto arg = _mm256_i32gather_epi64(ptr, idx, sizeof(u64));
    return mixImpl(arg);
  }

  JPP_ALWAYS_INLINE FastHash4 mixGatherNaive(const u64* base,
                                             const u32* indices) const
      noexcept {
    auto arg = _mm256_undefined_si256();
    arg[0] = base[indices[0]];
    arg[1] = base[indices[1]];
    arg[2] = base[indices[2]];
    arg[3] = base[indices[3]];
    return mixImpl(arg);
  }

  JPP_ALWAYS_INLINE FastHash4 mixImpl(__m256i arg) const noexcept {
    auto v = _mm256_xor_si256(state_, arg);
    v = Multiply64Bit(v, _mm256_set1_epi64x(SeaHashMult));
    auto x = _mm256_srli_epi64(v, 32);
    v = _mm256_xor_si256(v, x);
    return FastHash4{v};
  }

  FastHash4 maskCompact(u32 mask) const noexcept {
    auto v1 = _mm256_permutevar8x32_epi32(
        state_, _mm256_set_epi32(0, 2, 4, 6, 0, 2, 4, 6));
    v1 = _mm256_and_si256(v1, _mm256_set1_epi32(mask));
    return FastHash4{v1};
  }

  FastHash4 maskCompact2(u32 mask) const noexcept {
    auto v1 = _mm256_shuffle_epi32(state_, _MM_SHUFFLE(2, 0, 3, 1));
    v1 = _mm256_and_si256(v1, _mm256_set1_epi32(mask));
    return FastHash4{v1};
  }

  FastHash4 merge(FastHash4 o) const noexcept {
    auto t = _mm256_unpackhi_epi32(state_, o.state_);
    return FastHash4{t};
  }

  template <util::PrefetchHint hint, typename T>
  void prefetchGatherLo(const T* addr) const noexcept {
    _mm_prefetch(addr + _mm256_extract_epi32(state_, 0), hint);
    _mm_prefetch(addr + _mm256_extract_epi32(state_, 1), hint);
    _mm_prefetch(addr + _mm256_extract_epi32(state_, 2), hint);
    _mm_prefetch(addr + _mm256_extract_epi32(state_, 3), hint);
  }

  JPP_ALWAYS_INLINE void joinStore(FastHash4 other, u32 mask, u32* result) const
      noexcept {
    auto s = _mm256_blend_epi32(state_, other.state_,
                                JPP_BITS8(0, 0, 0, 0, 1, 1, 1, 1));
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(result), s);
  }

  JPP_ALWAYS_INLINE void jointMaskStore(FastHash4 other, u32 mask,
                                        u32* result) {
    auto v1 = _mm256_permutevar8x32_epi32(
        state_, _mm256_set_epi32(0, 2, 4, 6, 0, 2, 4, 6));
    auto v2 = _mm256_permutevar8x32_epi32(
        other.state_, _mm256_set_epi32(0, 2, 4, 6, 0, 2, 4, 6));
    auto s = _mm256_blend_epi32(v1, v2, JPP_BITS8(0, 0, 0, 0, 1, 1, 1, 1));
    s = _mm256_or_si256(s, _mm256_set1_epi32(mask));
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(result), s);
  }

  JPP_ALWAYS_INLINE void store(u64* result) const noexcept {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(result), state_);
  }

  JPP_ALWAYS_INLINE void store(u32* result) const noexcept {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(result), state_);
  }
};

#endif

#undef JPP_BITS8

}  // namespace hashing
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_SEAHASH_SIMD_H
