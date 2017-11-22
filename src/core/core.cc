//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core.h"

namespace jumanpp {
namespace core {

CoreHolder::CoreHolder(const spec::AnalysisSpec &spec,
                       const dic::DictionaryHolder &dic)
    : spec_{spec}, dic_{dic} {
  latticeCfg_.entrySize = static_cast<u32>(dic.entries().numFeatures());
  latticeCfg_.numFeaturePatterns =
      static_cast<u32>(spec.features.pattern.size());
}

Status CoreHolder::initialize(const features::StaticFeatureFactory *sff) {
  JPP_RETURN_IF_ERROR(analysis::makeMakers(*this, &unkMakers_));
  JPP_RETURN_IF_ERROR(features::makeFeatures(*this, sff, &features_));
  return Status::Ok();
}

}  // namespace core
}  // namespace jumanpp
