//
// Created by Arseny Tolmachev on 2017/10/12.
//

#include "ngram_computations.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace analysis {

void NgramStats::initialze(const features::FeatureRuntimeInfo *info) {
  num1Grams = 0;
  num2Grams = 0;
  num3Grams = 0;
  auto &ngramInfo = info->ngrams;
  for (auto &ng : ngramInfo) {
    auto order = ng.arguments.size();
    if (order == 1) {
      num1Grams += 1;
    } else if (order == 2) {
      num2Grams += 1;
    } else if (order == 3) {
      num3Grams += 1;
    }
  }
  maxNGrams = std::max({num1Grams, num2Grams, num3Grams});
}

void NgramScoreHolder::prepare(util::memory::ManagedAllocatorCore *alloc,
                               u32 maxNodes) {
  bufferT0_ = alloc->allocateBuf<float>(maxNodes);
  bufferT1_ = alloc->allocateBuf<float>(maxNodes);
  bufferT2_ = alloc->allocateBuf<float>(maxNodes);
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp