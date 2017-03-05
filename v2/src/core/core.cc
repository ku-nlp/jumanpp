//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core.h"

namespace jumanpp {
namespace core {

CoreHolder::CoreHolder(const CoreConfig &conf, const RuntimeInfo &runtime,
                       const dic::DictionaryHolder &dic)
    : cfg_{conf}, runtime_{runtime}, dic_{dic} {}

Status CoreHolder::initialize(const features::StaticFeatureFactory *sff) {
  JPP_RETURN_IF_ERROR(
      analysis::makeMakers(*this, runtime_.unkMakers, &unkMakers_));
  JPP_RETURN_IF_ERROR(features::makeFeatures(*this, sff, &features_));
  return Status::Ok();
}

}  // core
}  // jumanpp
