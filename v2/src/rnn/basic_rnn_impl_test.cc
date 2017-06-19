//
// Created by Arseny Tolmachev on 2017/06/19.
//

#include "mikolov_rnn_impl.h"
#include "testing/standalone_test.h"

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
    CHECK_OK(rnn.init(header, weights, maxent));
  }
};

TEST_CASE("rnn executor gets same scores in parts and in a whole") {
  SimpleRnn rnn;

  int beamSize = 3;
  int curItems = 6;

  rm::InferStepData isd{
      rnn.alloc->allocate2d<i32>(2, beamSize),
      rnn.alloc->allocateBuf<i32>(curItems),
      rnn.alloc->allocate2d<float>(beamSize, rnn.header.layerSize, 64),
      rnn.alloc->allocate2d<float>(curItems, rnn.header.layerSize, 64),
      rnn.alloc->allocate2d<float>(beamSize, curItems, 64)};

  rnn.rnn.calcScoresOn(isd, 0, 2);
  rnn.rnn.calcScoresOn(isd, 2, 2);
  rnn.rnn.calcScoresOn(isd, 4, 2);
}