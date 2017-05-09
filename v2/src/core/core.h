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

struct CoreConfig {
  i32 beamSize;
  i32 numScorers;
};

class CoreHolder {
  CoreConfig cfg_;
  const RuntimeInfo& runtime_;
  const dic::DictionaryHolder& dic_;
  analysis::UnkMakers unkMakers_;
  features::FeatureHolder features_;
  analysis::LatticeConfig latticeCfg_;

 public:
  CoreHolder(const CoreConfig& conf, const RuntimeInfo& runtime,
             const dic::DictionaryHolder& dic);

  Status initialize(const features::StaticFeatureFactory* sff);

  void updateCoreConfig(const CoreConfig& conf) {
    cfg_ = conf;
    latticeCfg_.beamSize = (u32)conf.beamSize;
    latticeCfg_.scoreCnt = (u32)conf.numScorers;
  }

  const analysis::LatticeConfig& latticeConfig() const { return latticeCfg_; }
  const dic::DictionaryHolder& dic() const { return dic_; }
  const RuntimeInfo& runtime() const { return runtime_; }
  const analysis::UnkMakers& unkMakers() const { return unkMakers_; }
  const features::FeatureHolder& features() const { return features_; }
};

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_CORE_H
