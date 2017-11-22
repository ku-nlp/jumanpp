//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "core/analysis/perceptron.h"
#include "core/analysis/lattice_types.h"
#include "core/impl/perceptron_io.h"
#include "util/logging.hpp"
#include "util/memory.hpp"
#include "util/serialization.h"

namespace jumanpp {
namespace core {
namespace analysis {

void HashedFeaturePerceptron::compute(util::MutableArraySlice<float> result,
                                      util::ConstSliceable<u32> ngrams) const {
  JPP_DCHECK(util::memory::IsPowerOf2(weights_.size()));
  auto weightobj = weights();
  u32 mask = static_cast<u32>(weightobj.size() - 1);
  for (int i = 0; i < ngrams.numRows(); ++i) {
    result.at(i) =
        impl::computeUnrolled4Perceptron(weightobj, ngrams.row(i), mask);
  }
}

void HashedFeaturePerceptron::add(util::ArraySlice<float> source,
                                  util::MutableArraySlice<float> result,
                                  util::ConstSliceable<u32> ngrams) const {
  JPP_DCHECK(util::memory::IsPowerOf2(weights_.size()));
  JPP_DCHECK_EQ(source.size(), result.size());
  auto weightobj = weights();
  u32 mask = static_cast<u32>(weightobj.size() - 1);
  auto total = ngrams.numRows();
  for (int i = 0; i < total; ++i) {
    auto src = source.at(i);
    auto add = impl::computeUnrolled4Perceptron(weightobj, ngrams.row(i), mask);
    result.at(i) = src + add;
  }
}

const size_t TWO_MEGS_FOR_FLOATS = 2 * 1024 * 1024 / sizeof(float);
class PerceptronState {
  util::memory::Manager manager_;
  std::unique_ptr<util::memory::PoolAlloc> alloc_;
  size_t numElems_;

 public:
  PerceptronState(size_t numElems)
      : manager_{std::max(numElems * sizeof(float), TWO_MEGS_FOR_FLOATS)},
        alloc_{manager_.core()},
        numElems_{numElems} {}

  const float* importDoubles(const float* data) {
    auto objs = numElems_;
    auto arr = alloc_->allocateArray<float>(objs);
    memcpy(arr, data, objs * sizeof(float));
    return arr;
  }
};

Status HashedFeaturePerceptron::load(const model::ModelInfo& model) {
  const model::ModelPart* savedPerc = nullptr;
  for (auto& part : model.parts) {
    if (part.kind == model::ModelPartKind::Perceprton) {
      savedPerc = &part;
      break;
    }
  }

  if (savedPerc == nullptr) {
    return Status::InvalidState()
           << "perceptron: saved model did not have perceptron attached";
  }

  if (savedPerc->data.size() != 2) {
    return Status::InvalidState()
           << "perceptron: saved model did not have exactly two parts";
  }

  auto& data = savedPerc->data;

  util::serialization::Loader ldr{data[0]};
  PerceptronInfo pi{};
  if (!ldr.load(&pi)) {
    return Status::InvalidState()
           << "perceptron: failed to load perceptron information";
  }

  if (pi.modelSizeExponent < 0) {
    return Status::InvalidState()
           << "perceptron: size exponent can't be negative";
  }

  if (pi.modelSizeExponent >= 64) {
    return Status::InvalidState()
           << "perceptron: size exponent must be lesser than 64";
  }

  auto dataSize = size_t{1} << pi.modelSizeExponent;

  StringPiece modelData = data[1];

  auto weightCount = modelData.size() / sizeof(float);

  if (weightCount != dataSize) {
    return Status::InvalidState()
           << "perceptron: slice size was not equal to model size in header";
  }

  auto weightData = reinterpret_cast<const float*>(modelData.begin());

  if (util::memory::Manager::supportHugePages()) {
    state_.reset(new PerceptronState{weightCount});
    weightData = state_->importDoubles(weightData);
  }

  util::ArraySlice<float> weightSlice{weightData, dataSize};

  weights_ = weightSlice;

  return Status::Ok();
}

HashedFeaturePerceptron::HashedFeaturePerceptron(
    const util::ArraySlice<float>& weights)
    : weights_{weights} {}

HashedFeaturePerceptron::HashedFeaturePerceptron() = default;
HashedFeaturePerceptron::~HashedFeaturePerceptron() = default;

WeightBuffer HashedFeaturePerceptron::weights() const {
#if 0
  const char* data = reinterpret_cast<const char*>(weights_.data());
  size_t sz = weights_.size();
  float min = -1;
  float step = 0.1;
  return WeightBuffer{data, sz, min, step};
#endif
  return weights_;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp