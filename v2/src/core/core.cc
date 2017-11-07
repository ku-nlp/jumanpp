//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core.h"

namespace jumanpp {
namespace core {

CoreHolder::CoreHolder(const spec::AnalysisSpec &spec,
                       const dic::DictionaryHolder &dic)
    : spec_{spec}, dic_{dic} {
  latticeCfg_.entrySize = dic.entries().numFeatures();
  latticeCfg_.numPrimitiveFeatures = spec.features.primitive.size();
  latticeCfg_.numFeaturePatterns = spec.features.pattern.size();
  latticeCfg_.numFinalFeatures = spec.features.ngram.size();
}

Status CoreHolder::initialize(const features::StaticFeatureFactory *sff) {
  JPP_RETURN_IF_ERROR(analysis::makeMakers(*this, &unkMakers_));
  JPP_RETURN_IF_ERROR(features::makeFeatures(*this, sff, &features_));
  return Status::Ok();
}

}  // namespace core
}  // namespace jumanpp
