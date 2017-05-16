//
// Created by Arseny Tolmachev on 2017/03/02.
//

#ifndef JUMANPP_CORE_H
#define JUMANPP_CORE_H

#include "core/analysis/lattice_config.h"
#include "core/analysis/unk_nodes.h"
#include "core/dictionary.h"
#include "core/features_api.h"
#include "core/runtime_info.h"

namespace jumanpp {
namespace core {

struct ScoringConfig {
  i32 beamSize;
  i32 numScorers;
};

class CoreHolder {
  const RuntimeInfo& runtime_;
  const dic::DictionaryHolder& dic_;
  analysis::UnkMakers unkMakers_;
  features::FeatureHolder features_;
  analysis::LatticeConfig latticeCfg_;

 public:
  CoreHolder(const RuntimeInfo& runtime, const dic::DictionaryHolder& dic);

  Status initialize(const features::StaticFeatureFactory* sff);

  analysis::LatticeConfig latticeConfig(const ScoringConfig& core) const {
    analysis::LatticeConfig copy = latticeCfg_;
    copy.beamSize = (u32)core.beamSize;
    copy.scoreCnt = (u32)core.numScorers;
    return copy;
  }

  const dic::DictionaryHolder& dic() const { return dic_; }
  const RuntimeInfo& runtime() const { return runtime_; }
  const analysis::UnkMakers& unkMakers() const { return unkMakers_; }
  const features::FeatureHolder& features() const { return features_; }
};

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_CORE_H
