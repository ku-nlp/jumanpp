//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_RNN_SCORER_H
#define JUMANPP_RNN_SCORER_H

#include "core/analysis/score_api.h"
#include "rnn/mikolov_rnn.h"
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {

namespace core {
namespace analysis {
namespace rnn {

struct RnnHolderState;

class RnnHolder : public ScorerFactory {
  std::unique_ptr<RnnHolderState> impl_;
  friend struct RnnScorerState;

 public:
  RnnHolder();
  ~RnnHolder();
  Status init(const jumanpp::rnn::mikolov::MikolovModelReader& model);
  Status load(const model::ModelInfo& model) override;
  Status makeInstance(std::unique_ptr<ScoreComputer>* result) override;
};

struct RnnScorerState;

class RnnScorer : public ScoreComputer {
  std::unique_ptr<RnnScorerState> state_;

 public:
  ~RnnScorer();
  Status init(const RnnHolder& holder);
  void preScore(Lattice* l);
  bool scoreBoundary(i32 scorerIdx, Lattice* l, i32 boundary);
};

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_SCORER_H
