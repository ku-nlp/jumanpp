//
// Created by Arseny Tolmachev on 2017/02/17.
//

#ifndef JUMANPP_COMMON_HPP
#define JUMANPP_COMMON_HPP

#if defined(_WIN32_WINNT) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

#ifdef _MSC_VER

#define JPP_ALWAYS_INLINE __forceinline
#define JPP_NO_INLINE __declspec(noinline)

#define JPP_UNLIKELY(x) x
#define JPP_LIKELY(x) x
#else  // GCC & Clang
#define JPP_ALWAYS_INLINE __attribute__((always_inline))
#define JPP_NO_INLINE __attribute__((noinline))

#define JPP_UNLIKELY(x) __builtin_expect((x), 0)
#define JPP_LIKELY(x) __builtin_expect(!!(x), 1)
#endif  //_MSC_VER

#if __APPLE__ && __MACH__
#define __unix__ 1
#endif

#ifdef __clang__
#define JPP_NODISCARD __attribute__((warn_unused_result))
#else
#define JPP_NODISCARD
#endif

namespace jumanpp {
namespace util {

// Prefetching support
//
// Defined behavior on some of the uarchs:
// PREFETCH_HINT_T0:
//   prefetch to all levels of the hierarchy (except on p4: prefetch to L2)
// PREFETCH_HINT_NTA:
//   p4: fetch to L2, but limit to 1 way (out of the 8 ways)
//   core: skip L2, go directly to L1
//   k8 rev E and later: skip L2, can go to either of the 2-ways in L1
// Imported from TensorFlow.
enum PrefetchHint {
  PREFETCH_HINT_T0 = 3,  // More temporal locality
  PREFETCH_HINT_T1 = 2,
  PREFETCH_HINT_T2 = 1,  // Less temporal locality
  PREFETCH_HINT_NTA = 0  // No temporal locality
};
template <PrefetchHint hint>
JPP_ALWAYS_INLINE void prefetch(const void* x);

// ---------------------------------------------------------------------------
// Inline implementation
// ---------------------------------------------------------------------------
template <PrefetchHint hint>
inline JPP_ALWAYS_INLINE void prefetch(const void* x) {
#if defined(__llvm__) || defined(__GNUC__)
  __builtin_prefetch(x, 0, hint);
#elif defined(_WIN32_WINNT) && (defined(_M_IX86) || defined(_M_X64))
  _mm_prefetch(static_cast<const char*>(x), hint);
#else
// You get no effect.  Feel free to add more sections above.
#endif
}

#define JPP_RET_CHECK(x)                  \
  {                                       \
    if (JPP_UNLIKELY(!(x))) return false; \
  }

namespace port {
constexpr bool kLittleEndian = true;
}

}  // namespace util
}  // namespace jumanpp

#include "util/assert.h"

#endif  // JUMANPP_COMMON_HPP
