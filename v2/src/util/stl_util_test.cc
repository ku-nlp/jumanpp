//
// Created by Arseny Tolmachev on 2017/10/20.
//
#include "stl_util.h"
#include <random>
#include "debug_output.h"
#include "logging.hpp"
#include "testing/standalone_test.h"

namespace u = jumanpp::util;

template <typename It, typename Comp>
It part_step(It start, It end, Comp comp) {
  auto sz = std::distance(start, end);
  if (sz == 1) {
    return end;
  }
  if (sz == 2) {
    auto n = start;
    ++n;
    if (comp(*n, *start)) {
      std::swap(*n, *start);
    }
    return n;
  }
  if (sz == 3) {
    auto n0 = start;
    auto n1 = n0;
    ++n1;
    auto n2 = n1;
    ++n2;
    if (comp(*n1, *n0)) {
      std::swap(*n1, *n0);
    }
    if (comp(*n2, *n1)) {
      std::swap(*n2, *n1);
    }
    if (comp(*n1, *n0)) {
      std::swap(*n1, *n0);
    }
    return n1;
  }
  It pivot{start};
  std::advance(pivot, sz / 2);
  auto s0 = start;
  auto e0 = end;
  LOG_TRACE() << jumanpp::VOut(s0, e0) << " : Start";
  --end;
  std::swap(*pivot, *end);
  pivot = end;
  --end;
  while (start != end) {
    if (comp(*start, *pivot)) {
      LOG_TRACE() << jumanpp::VOut(s0, e0) << " : Move start";
      ++start;
    } else {
      std::swap(*start, *end);
      LOG_TRACE() << jumanpp::VOut(s0, e0) << " : Swap end";
      --end;
    }
  }
  //++end;
  std::swap(*pivot, *end);
  LOG_TRACE() << jumanpp::VOut(s0, e0) << " : Final";
  return end;
}

template <typename It, typename Comp>
It partition(It start, It end, Comp comp, size_t minSize, size_t maxSize) {
  auto sz = std::distance(start, end);
  JPP_DCHECK_LE(minSize, sz);
  JPP_DCHECK_LE(minSize, maxSize);
  JPP_DCHECK_LE(maxSize, sz);

  while (true) {
    It mid = part_step(start, end, comp);
    sz = std::distance(start, mid);
    if (minSize <= sz && sz <= maxSize) {
      return mid;
    }

    if (sz > maxSize) {
      end = mid;
      continue;
    }

    sz += 1;
    start = mid;
    ++start;
    minSize -= sz;
    maxSize -= sz;

    if (minSize == 0) {
      return start;
    }
  }
};

TEST_CASE("can partition a simple array") {
  std::vector<int> v{10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  auto r = u::partition(v.begin(), v.end(), std::less<>(), 3, 4);
  CHECK(std::distance(v.begin(), r) < 5);
  CHECK(v[0] < 3);
  CHECK(v[1] < 3);
  CHECK(v[2] < 3);
}

TEST_CASE("can partition a shuffled array") {
  std::vector<int> v{10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  std::minstd_rand rng{0xdead};
  for (int i = 0; i < 100; ++i) {
    CAPTURE(i);
    std::shuffle(v.begin(), v.end(), rng);
    auto r = u::partition(v.begin(), v.end(), std::less<>(), 3, 4);
    CHECK(std::distance(v.begin(), r) < 5);
    CHECK(v[0] < 3);
    CHECK(v[1] < 3);
    CHECK(v[2] < 3);
  }
}
