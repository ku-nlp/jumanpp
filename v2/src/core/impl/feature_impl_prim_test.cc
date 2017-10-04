//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include "core/impl/feature_impl_prim.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::features::impl;
using namespace jumanpp;

TEST_CASE("CopyPrimFeatureImpl compiles in both usage paterns") {
  constexpr CopyPrimFeatureImpl f{0, 0};
  DynamicPrimitiveFeature<CopyPrimFeatureImpl> impl;
}

TEST_CASE("ProvidedPrimFeatureImpl compiles in both usage paterns") {
  constexpr ProvidedPrimFeatureImpl f{0, 0};
  DynamicPrimitiveFeature<ProvidedPrimFeatureImpl> impl;
}

TEST_CASE("MatchDicPrimFeatureImpl compiles in both usage patterns") {
  static constexpr i32 array[] = {0, 1, 2};
  constexpr MatchDicPrimFeatureImpl f{0, 0, array};
  DynamicPrimitiveFeature<MatchDicPrimFeatureImpl> impl;
}

TEST_CASE("MatchAnyDicPrimFeatureImpl compiles in both usage patterns") {
  static constexpr i32 array[] = {0, 1, 2};
  constexpr MatchListElemPrimFeatureImpl f{0, 0, array};
  DynamicPrimitiveFeature<MatchListElemPrimFeatureImpl> impl;
}