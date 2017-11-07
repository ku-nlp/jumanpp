//
// Created by Arseny Tolmachev on 2017/03/05.
//

#include "features_api.h"
#include "core/core.h"
#include "core/impl/feature_impl_pattern.h"
#include "impl/feature_impl_combine.h"
#include "impl/feature_impl_compute.h"
#include "impl/feature_impl_ngram_partial.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace features {

Status makeFeatures(const CoreHolder &info, const StaticFeatureFactory *sff,
                    FeatureHolder *result) {
  impl::FeatureConstructionContext fcc{&info.dic().fields()};
  auto &runtimeFeatures = info.spec().features;
  auto dynPattern = new impl::PatternDynamicApplyImpl;
  result->patternDynamic.reset(dynPattern);
  JPP_RETURN_IF_ERROR(dynPattern->initialize(&fcc, info.spec().features));

  auto dynNgram = new impl::NgramDynamicFeatureApply;
  result->ngramDynamic.reset(dynNgram);
  for (auto &nf : runtimeFeatures.ngram) {
    JPP_RETURN_IF_ERROR(dynNgram->addChild(nf));
  }

  auto partNgram = new impl::PartialNgramDynamicFeatureApply;
  result->ngramPartialDynamic.reset(partNgram);
  for (auto &nf : runtimeFeatures.ngram) {
    JPP_RETURN_IF_ERROR(partNgram->addChild(nf));
  }

  if (sff != nullptr) {
    if (true) {  // TODO: check hashes
      result->patternStatic.reset(sff->pattern());
      result->ngramStatic.reset(sff->ngram());
      result->ngramPartialStatic.reset(sff->ngramPartial());
    } else {
      LOG_WARN()
          << "hashes did not match, compiled static features can't be used";
    }
  }

  if (result->ngramStatic) {
    result->ngram = result->ngramStatic.get();
  } else {
    result->ngram = result->ngramDynamic.get();
  }

  if (result->ngramPartialStatic) {
    result->ngramPartial = result->ngramPartialStatic.get();
  } else {
    result->ngramPartial = result->ngramPartialDynamic.get();
  }

  return Status::Ok();
}

Status FeatureHolder::validate() const {
  if (patternDynamic == nullptr) {
    return JPPS_INVALID_STATE << "Features: pattern dynamic was null";
  }

  if (ngram == nullptr) {
    return JPPS_INVALID_STATE << "Features: ngram was null";
  }

  if (ngramPartial == nullptr) {
    return JPPS_INVALID_STATE << "Features: ngramPartial was null";
  }

  return Status::Ok();
}

FeatureHolder::FeatureHolder() = default;
FeatureHolder::~FeatureHolder() = default;

}  // namespace features
}  // namespace core
}  // namespace jumanpp