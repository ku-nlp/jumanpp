//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_RNN_SCORER_H
#define JUMANPP_RNN_SCORER_H

#include "core/analysis/score_api.h"
#include "core/dictionary.h"
#include "rnn/mikolov_rnn.h"
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

struct RnnHolderState;

struct RnnInferenceConfig {
  float nceBias = 0;
  float unkConstantTerm = -6.0f;
  float unkLengthPenalty = -0.5f;
  float perceptronWeight = 1.0f;
  float rnnWeight = 1.0f;
  std::string unkSymbol;
  u32 unkId = 0;

  bool operator==(const RnnInferenceConfig& other) const {
    return nceBias == other.nceBias &&
           unkConstantTerm == other.unkConstantTerm &&
           unkLengthPenalty == other.unkLengthPenalty &&
           perceptronWeight == other.perceptronWeight &&
           rnnWeight == other.rnnWeight;
  }

  bool isDefault() const {
    RnnInferenceConfig defCfg;
    return defCfg == *this;
  }
};

class RnnHolder : public ScorerFactory {
  std::unique_ptr<RnnHolderState> impl_;
  friend struct RnnScorerState;

 public:
  RnnHolder();
  ~RnnHolder() override;
  Status init(const jumanpp::rnn::mikolov::MikolovModelReader& model,
              const core::dic::DictionaryHolder& dic, StringPiece field);
  void setConfig(const RnnInferenceConfig& conf);
  const RnnInferenceConfig& config() const;
  Status load(const model::ModelInfo& model) override;
  Status makeInfo(model::ModelInfo* result);
  Status makeInstance(std::unique_ptr<ScoreComputer>* result) override;
};

struct RnnScorerState;

class RnnScorer : public ScoreComputer {
  std::unique_ptr<RnnScorerState> state_;

 public:
  ~RnnScorer() override;
  Status init(const RnnHolder& holder);
  void preScore(Lattice* l, ExtraNodesContext* xtra) override;
  bool scoreBoundary(i32 scorerIdx, Lattice* l, i32 boundary) override;
};

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_SCORER_H
