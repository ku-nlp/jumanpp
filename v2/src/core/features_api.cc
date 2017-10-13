//
// Created by Arseny Tolmachev on 2017/03/05.
//

#include "features_api.h"
#include "core/core.h"
#include "impl/feature_impl_combine.h"
#include "impl/feature_impl_compute.h"
#include "impl/feature_impl_prim.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace features {

Status makeFeatures(const CoreHolder &info, const StaticFeatureFactory *sff,
                    FeatureHolder *result) {
  impl::FeatureConstructionContext fcc{&info.dic().fields()};
  std::unique_ptr<impl::PrimitiveFeaturesDynamicApply> dynamic{
      new impl::PrimitiveFeaturesDynamicApply};
  auto &runtimeFeatures = info.runtime().features;
  JPP_RETURN_IF_ERROR(dynamic->initialize(&fcc, runtimeFeatures.primitive));
  result->primitiveDynamic = std::move(dynamic);

  auto dynComp = new impl::ComputeFeatureDynamicApply;
  result->computeDynamic.reset(dynComp);
  for (auto &cf : runtimeFeatures.compute) {
    dynComp->makeFeature(cf);
  }

  auto dynPat = new impl::PatternDynamicApplyImpl;
  result->patternDynamic.reset(dynPat);
  for (auto &patf : runtimeFeatures.patterns) {
    dynPat->addChild(patf);
  }

  auto dynNgram = new impl::NgramDynamicFeatureApply;
  result->ngramDynamic.reset(dynNgram);
  for (auto &nf : runtimeFeatures.ngrams) {
    JPP_RETURN_IF_ERROR(dynNgram->addChild(nf));
  }

  if (sff != nullptr) {
    if (true) {  // TODO: check hashes
      result->primitiveStatic.reset(sff->primitive());
      result->computeStatic.reset(sff->compute());
      result->patternStatic.reset(sff->pattern());
      result->ngramStatic.reset(sff->ngram());
      result->ngramPartialStatic.reset(sff->ngramPartial());
    } else {
      LOG_WARN()
          << "hashes did not match, compiled static features can't be used";
    }
  }

  if (result->primitiveStatic) {
    result->primitive = result->primitiveStatic.get();
  } else {
    result->primitive = result->primitiveDynamic.get();
  }

  if (result->computeStatic) {
    result->compute = result->computeStatic.get();
  } else {
    result->compute = result->computeDynamic.get();
  }

  if (result->patternStatic) {
    result->pattern = result->patternStatic.get();
  } else {
    result->pattern = result->patternDynamic.get();
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
  if (primitive == nullptr) {
    return JPPS_INVALID_STATE << "Features: primitive was null";
  }

  if (compute == nullptr) {
    return JPPS_INVALID_STATE << "Features: compute was null";
  }

  if (pattern == nullptr) {
    return JPPS_INVALID_STATE << "Features: pattern was null";
  }

  if (ngram == nullptr) {
    return JPPS_INVALID_STATE << "Features: ngram was null";
  }

  return Status::Ok();
}

FeatureHolder::FeatureHolder() = default;
FeatureHolder::~FeatureHolder() = default;

}  // namespace features
}  // namespace core
}  // namespace jumanpp