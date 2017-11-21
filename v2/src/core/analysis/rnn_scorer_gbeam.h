//
// Created by Arseny Tolmachev on 2017/11/20.
//

#ifndef JUMANPP_RNN_SCORER_GBEAM_H
#define JUMANPP_RNN_SCORER_GBEAM_H

#include <memory>
#include "core/analysis/rnn_scorer.h"

namespace jumanpp {
namespace core {
namespace analysis {

struct GbeamRnnState;
struct GbeamRnnFactoryState;

class Lattice;
class ExtraNodesContext;

class RnnScorerGbeamFactory : ScorerFactory {
  std::unique_ptr<GbeamRnnFactoryState> state_;

 public:
  RnnScorerGbeamFactory();
  ~RnnScorerGbeamFactory() override;

 private:
  Status load(const model::ModelInfo& model) override;
  Status makeInstance(std::unique_ptr<ScoreComputer>* result) override;
};

class RnnScorerGbeam {
  std::unique_ptr<GbeamRnnState> state_;

 public:
  Status scoreLattice(Lattice* l, const ExtraNodesContext* xtra, u32 scorerIdx);
  RnnScorerGbeam();
  ~RnnScorerGbeam();
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_SCORER_GBEAM_H
