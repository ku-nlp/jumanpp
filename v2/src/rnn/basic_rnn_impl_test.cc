//
// Created by Arseny Tolmachev on 2017/06/19.
//

#include <random>
#include "mikolov_rnn_impl.h"
#include "testing/standalone_test.h"
#include "util/debug_output.h"

using namespace jumanpp;

namespace rm = rnn::mikolov;

struct SimpleRnn {
  util::memory::Manager mgr{1024 * 1024};
  std::shared_ptr<util::memory::ManagedAllocatorCore> alloc{mgr.core()};

  rm::MikolovRnnModelHeader header{16, 3, 150, 100, 0};

  rm::MikolovRnn rnn;

  SimpleRnn() {
    auto& h = header;
    auto weights = alloc->allocateBuf<float>(h.layerSize * h.layerSize, 64);
    auto maxent = alloc->allocateBuf<float>(h.maxentSize, 64);
    for (int i = 0; i < maxent.size(); ++i) {
      maxent[i] = i;
    }

    for (int j = 0; j < weights.size(); ++j) {
      int asd = weights.size();
      weights.at(j) = (j - asd / 2) * 0.01f * (j % 3 == 0 ? -1 : 1);
    }

    CHECK_OK(rnn.init(header, weights, maxent));
  }

  template <typename T>
  util::Sliceable<T> const2d(size_t rows, size_t cols, T val) {
    auto data = alloc->allocate2d<T>(rows, cols, 64);
    util::fill(data, val);
    return data;
  }

  template <typename T, typename Distr = typename std::conditional<
                            std::is_floating_point<T>::value,
                            std::uniform_real_distribution<T>,
                            std::uniform_int_distribution<T>>::type>
  util::Sliceable<T> rand2d(size_t rows, size_t cols, T lower, T higer) {
    auto data = alloc->allocate2d<T>(rows, cols, 64);
    std::minstd_rand rng{0xdeadbeef};
    Distr gen{lower, higer};
    for (size_t i = 0; i < data.size(); ++i) {
      data[i] = gen(rng);
    }
    return data;
  }
};

template <typename T>
std::initializer_list<T> il(std::initializer_list<T> lst) {
  return lst;
}

template <typename C, typename Cmp>
struct EqSmt {
  const C& obj;

  explicit EqSmt(const C &obj) : obj(obj) {}

  template <typename C1>
  bool operator==(const C1& o) const noexcept {
    auto s1 = obj.size();
    auto s2 = o.size();
    if (s1 != s2) {
      return false;
    }
    auto d1 = obj.data();
    auto d2 = o.data();
    Cmp cmp{};
    for (size_t i = 0; i < s1; ++i) {
      if (!cmp(d1[i], d2[i])) {
        return false;
      }
    }
    return true;
  }
};

template <typename C, typename Cmp>
std::ostream& operator<<(std::ostream& os, const EqSmt<C, Cmp>& s) {
  os << s.obj;
  return os;
}

template <typename T, typename Cmp = std::equal_to<T>>
EqSmt<util::Sliceable<T>, Cmp> eqf(const util::Sliceable<T>& obj) {
  return EqSmt<util::Sliceable<T>, Cmp>{obj};
}


TEST_CASE("rnn executor gets same scores in parts and in a whole") {
  SimpleRnn rnn;

  int beamSize = 3;
  int curItems = 6;

  rm::ContextStepData cstep{
      rnn.const2d<float>(beamSize, rnn.header.layerSize, std::numeric_limits<float>::signaling_NaN()),
      rnn.rand2d<float>(1, rnn.header.layerSize, -0.1f, 0.1f).row(0),
      rnn.const2d<float>(beamSize, rnn.header.layerSize, 0.1f)
  };

  rnn.rnn.calcNewContext(cstep);

  auto isdCtxIds = rnn.alloc->allocate2d<i32>(2, beamSize);
  util::copy_buffer(il({1,2,3,4,5,6}), isdCtxIds);
  auto isdItems = rnn.alloc->allocateBuf<i32>(curItems);
  util::copy_buffer(il({7,8,9,10,11,12}), isdItems);
  rm::InferStepData isd{
      isdCtxIds,
      isdItems,
      cstep.curContext,
      rnn.rand2d<float>(curItems, rnn.header.layerSize, -0.1f, 0.f),
      rnn.const2d<float>(beamSize, curItems, std::numeric_limits<float>::signaling_NaN())};

  rnn.rnn.calcScoresOn(isd, 0, 2);
  rnn.rnn.calcScoresOn(isd, 4, 2);
  rnn.rnn.calcScoresOn(isd, 2, 2);

  rm::StepData stepData{
      isdCtxIds,
      isdItems,
      cstep.prevContext,
      cstep.leftEmbedding,
      isd.rightEmbeddings,
      rnn.const2d<float>(beamSize, rnn.header.layerSize, std::numeric_limits<float>::signaling_NaN()),
      rnn.const2d<float>(beamSize, curItems, std::numeric_limits<float>::signaling_NaN())
  };

  rnn.rnn.apply(&stepData);

  CHECK(eqf(stepData.beamContext) == cstep.curContext);
  CHECK(eqf(stepData.scores) == isd.scores);
}
