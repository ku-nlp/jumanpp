//
// Created by Arseny Tolmachev on 2017/02/17.
//

#ifndef JUMANPP_COMMON_HPP
#define JUMANPP_COMMON_HPP

#define JPP_ALWAYS_INLINE __attribute__((always_inline))

#define JPP_LIKELY(x) __builtin_expect((x), 1)
#define JPP_UNLIKELY(x) __builtin_expect((x), 0)

#endif  // JUMANPP_COMMON_HPP
