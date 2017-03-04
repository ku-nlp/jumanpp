//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_RUNTIME_INFO_H
#define JUMANPP_RUNTIME_INFO_H

#include "core/analysis/unk_maker_types.h"
#include "impl/feature_types.h"

namespace jumanpp {
namespace core {

struct RuntimeInfo {
  features::FeatureRuntimeInfo features;
  analysis::UnkMakers unkMakers;
};

}  // core
}  // jumanpp

#endif  // JUMANPP_RUNTIME_INFO_H
