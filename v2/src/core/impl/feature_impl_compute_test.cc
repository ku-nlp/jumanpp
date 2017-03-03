//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "feature_impl_compute.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::features::impl;
using namespace jumanpp::core::spec;
using namespace jumanpp;

TEST_CASE("match-compute binary search works") {
  static constexpr i32 refs[] = {0, 1};
  static constexpr i32 matchData[] = {0, 1, 0, 2, 1, 2, 5, 6, 7, 2};
  static constexpr i32 trueBr[] = {0};
  static constexpr i32 falseBr[] = {1};
  static constexpr MatchTupleComputeFeatureImpl impl{0, refs, matchData, trueBr,
                                                     falseBr};
  CHECK(impl.matches({7, 2}));
  CHECK(impl.matches({0, 1}));
  CHECK(impl.matches({5, 6}));
  CHECK(impl.matches({0, 2}));
  CHECK(impl.matches({1, 2}));
  CHECK_FALSE(impl.matches({0, 5}));
  CHECK_FALSE(impl.matches({1, 5}));
}