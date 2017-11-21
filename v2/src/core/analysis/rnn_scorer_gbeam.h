//
// Created by Arseny Tolmachev on 2017/11/20.
//

#ifndef JUMANPP_RNN_SCORER_GBEAM_H
#define JUMANPP_RNN_SCORER_GBEAM_H

#include <memory>
#include "core/analysis/rnn_scorer.h"
#include "core/analysis/score_api.h"

namespace jumanpp {
namespace core {
namespace dic {
class DictionaryHolder;
}
namespace analysis {

struct GbeamRnnState;
struct GbeamRnnFactoryState;

class Lattice;
class ExtraNodesContext;

class RnnScorerGbeamFactory : public ScorerFactory {
  std::unique_ptr<GbeamRnnFactoryState> state_;

 public:
  RnnScorerGbeamFactory();
  ~RnnScorerGbeamFactory() override;
  Status make(StringPiece rnnModelPath, const dic::DictionaryHolder& dic,
              const rnn::RnnInferenceConfig& config);
  Status load(const model::ModelInfo& model) override;
  Status makeInfo(model::ModelInfo* info);
  Status makeInstance(std::unique_ptr<ScoreComputer>* result) override;
  void setConfig(const rnn::RnnInferenceConfig& config);
  const rnn::RnnInferenceConfig& config() const;
};

class RnnScorerGbeam : public ScoreComputer {
  std::unique_ptr<GbeamRnnState> state_;

 public:
  Status scoreLattice(Lattice* l, const ExtraNodesContext* xtra, u32 scorerIdx);
  RnnScorerGbeam();
  ~RnnScorerGbeam();

  friend class RnnScorerGbeamFactory;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_SCORER_GBEAM_H
