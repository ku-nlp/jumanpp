//
// Created by Arseny Tolmachev on 2018/01/19.
//

#ifndef JUMANPP_SCORE_PLUGIN_H
#define JUMANPP_SCORE_PLUGIN_H

#include "core/analysis/lattice_config.h"

namespace jumanpp {
namespace core {
namespace analysis {

class ScorePlugin {
 public:
  virtual ~ScorePlugin() = 0;
  virtual bool updateScore(const Lattice* l, const ConnectionPtr& ptr,
                           float* score) const = 0;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SCORE_PLUGIN_H
