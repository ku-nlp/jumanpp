//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "perceptron.h"
#include "testing/standalone_test.h"

using namespace jumanpp;
using namespace jumanpp::core::analysis;

TEST_CASE("perceptron impl computes simple sums") {
  float weights[] = {
      0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f,
      0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f,
  };
  util::ArraySlice<float> wslice{weights};
  CHECK(wslice.size() == 16);
  auto compute = [&](util::ArraySlice<u32> v) -> float {
    return impl::computeUnrolled4Perceptron(weights, v, 16 - 1);
  };

  CHECK(compute({1}) == Approx(0.1f));
  CHECK(compute({1, 2}) == Approx(0.2f));
  CHECK(compute({1, 2, 3}) == Approx(0.3f));
  CHECK(compute({1, 2, 5, 9}) == Approx(0.4f));
  CHECK(compute({1, 7, 5, 9, 3}) == Approx(0.5f));
}

TEST_CASE("perceptron impl computes positioned sums") {
  float weights[] = {
      1e0f, 1e1f, 1e2f,  1e3f,  1e4f,  1e5f,  1e6f,  1e7f,
      1e8f, 1e9f, 1e10f, 1e11f, 1e12f, 1e13f, 1e14f, 1e15f,
  };
  util::ArraySlice<float> wslice{weights};
  CHECK(wslice.size() == 16);
  auto compute = [&](util::ArraySlice<u32> v) -> float {
    return impl::computeUnrolled4Perceptron(weights, v, 16 - 1);
  };

  CHECK(compute({1}) == Approx(10.f));
  CHECK(compute({1, 2}) == Approx(110.f));
  CHECK(compute({1, 2, 3}) == Approx(1110.f));
  CHECK(compute({6, 7, 5, 9}) == Approx(10111'00000.f));
  CHECK(compute({8, 7, 5, 9, 3}) == Approx(11101'01000.f));
}