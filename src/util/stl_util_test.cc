//
// Created by Arseny Tolmachev on 2017/10/20.
//
#include "stl_util.h"
#include <random>
#include "debug_output.h"
#include "logging.hpp"
#include "testing/standalone_test.h"

namespace u = jumanpp::util;

TEST_CASE("can partition a simple array") {
  std::vector<int> v{10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  auto r = u::partition(v.begin(), v.end(), std::less<>(), 3, 4);
  CHECK(std::distance(v.begin(), r) < 5);
  CHECK(v[0] < 3);
  CHECK(v[1] < 3);
  CHECK(v[2] < 3);
}

TEST_CASE("can partition a shuffled array (3,4)") {
  std::vector<int> v{10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  std::minstd_rand rng{0xdead};
  for (int i = 0; i < 100; ++i) {
    CAPTURE(i);
    std::shuffle(v.begin(), v.end(), rng);
    auto r = u::partition(v.begin(), v.end(), std::less<>(), 3, 4);
    auto dist = std::distance(v.begin(), r);
    CHECK(dist < 5);
    CHECK(v[0] <= 3);
    CHECK(v[1] <= 3);
    CHECK(v[2] <= 3);
    if (dist >= 4) {
      CHECK(v[3] <= 4);
    }
  }
}

TEST_CASE("can partition a shuffled array (2,3)") {
  std::vector<int> v{10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  std::minstd_rand rng{0xdead};
  for (int i = 0; i < 100; ++i) {
    CAPTURE(i);
    std::shuffle(v.begin(), v.end(), rng);
    auto r = u::partition(v.begin(), v.end(), std::less<>(), 2, 3);
    auto dist = std::distance(v.begin(), r);
    CHECK(dist < 4);
    CHECK(v[0] <= 2);
    CHECK(v[1] <= 2);
    if (dist >= 3) {
      CHECK(v[3] <= 3);
    }
  }
}

TEST_CASE("can partition a shuffled array 20: (4,7)") {
  std::vector<int> v{
      10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  };
  std::minstd_rand rng{0xdead};
  for (int i = 0; i < 300; ++i) {
    CAPTURE(i);
    std::shuffle(v.begin(), v.end(), rng);
    auto r = u::partition(v.begin(), v.end(), std::less<>(), 4, 7);
    auto dist = std::distance(v.begin(), r);
    CAPTURE(dist);
    CHECK(dist < 8);
    CHECK(v[0] < dist);
    CHECK(v[1] < dist);
    CHECK(v[2] < dist);
    CHECK(v[3] < dist);
    if (dist > 4) {
      CHECK(v[4] < dist);
    }
    if (dist > 5) {
      CHECK(v[5] < dist);
    }
    if (dist > 6) {
      CHECK(v[6] < dist);
    }
  }
}
