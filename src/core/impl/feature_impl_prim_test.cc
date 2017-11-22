//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include "core/impl/feature_impl_prim.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::features::impl;
using namespace jumanpp;

TEST_CASE("CopyPrimFeatureImpl compiles in both usage paterns") {
  constexpr CopyPrimFeatureImpl f{0};
  DynamicPrimitiveFeature<CopyPrimFeatureImpl> impl{0};
}

TEST_CASE("ProvidedPrimFeatureImpl compiles in both usage paterns") {
  constexpr ProvidedPrimFeatureImpl f{0};
  DynamicPrimitiveFeature<ProvidedPrimFeatureImpl> impl{0};
}