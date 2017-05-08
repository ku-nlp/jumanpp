//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "core/analysis/perceptron.h"
#include "core/analysis/lattice_types.h"
#include "core/impl/perceptron_io.h"
#include "util/serialization.h"

namespace jumanpp {
namespace core {
namespace analysis {

void HashedFeaturePerceptron::compute(util::MutableArraySlice<float> result,
                                      util::Sliceable<u32> ngrams) const {
  JPP_DCHECK(util::memory::IsPowerOf2(weights_.size()));
  u32 mask = static_cast<u32>(weights_.size() - 1);
  for (int i = 0; i < ngrams.numRows(); ++i) {
    result.at(i) =
        impl::computeUnrolled4Perceptron(weights_, ngrams.row(i), mask);
  }
}

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

  auto sliceSize = modelData.size() / sizeof(float);

  if (sliceSize != dataSize) {
    return Status::InvalidState()
           << "perceptron: slice size was not equal to model size in header";
  }

  util::ArraySlice<float> weightData{
      reinterpret_cast<const float*>(modelData.begin()), dataSize};

  weights_ = weightData;

  return Status::Ok();
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp