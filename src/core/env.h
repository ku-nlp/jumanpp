//
// Created by Arseny Tolmachev on 2017/05/10.
//

#ifndef JUMANPP_ENV_H
#define JUMANPP_ENV_H

#include "core/analysis/analyzer.h"
#include "core/analysis/perceptron.h"
#include "core/analysis/rnn_scorer_gbeam.h"
#include "core/impl/model_io.h"

namespace jumanpp {
namespace core {

class JumanppEnv {
  ScoringConfig scoringConf_{1, 0};
  model::FilesystemModel modelFile_;
  model::ModelInfo modelInfo_;
  dic::BuiltDictionary dicBldr_;
  dic::DictionaryHolder dicHolder_;
  std::unique_ptr<core::CoreHolder> core_;

  analysis::AnalyzerConfig analyzerConfig_{};

  analysis::HashedFeaturePerceptron perceptron_;
  analysis::RnnScorerGbeamFactory rnnHolder_;
  analysis::ScorerDef scorers_;

 public:
  Status loadModel(StringPiece filename);

  Status initFeatures(const features::StaticFeatureFactory* pFactory);
  void setBeamSize(u32 size);

  const CoreHolder* coreHolder() const { return core_.get(); }
  const analysis::ScorerDef* scorers() const { return &scorers_; }
  const spec::AnalysisSpec& spec() const { return dicBldr_.spec; }
  bool hasPerceptronModel() const;

  bool hasRnnModel() const;
  void setRnnConfig(const analysis::rnn::RnnInferenceConfig& rnnConf);
  void setRnnHolder(analysis::RnnScorerGbeamFactory* holder);

  Status makeAnalyzer(analysis::Analyzer* result) const {
    if (!hasPerceptronModel()) {
      return Status::InvalidState()
             << "loaded model (" << modelFile_.name() << ") was not trained";
    }
    JPP_RETURN_IF_ERROR(result->initialize(coreHolder(), analyzerConfig_,
                                           scoringConf_, &scorers_));
    return Status::Ok();
  }

  model::ModelInfo modelInfoCopy() const { return modelInfo_; }

  void setGlobalBeam(i32 globalBeam, i32 rightCheck, i32 rightBeam);
  void setAutoBeam(i32 base, i32 step, i32 max);

  const analysis::FeatureScorer* featureScorer() const { return &perceptron_; }
};

class OutputFormat {
 public:
  virtual ~OutputFormat() = default;
  virtual Status format(const core::analysis::Analyzer& analyzer,
                        StringPiece comment) = 0;
  virtual StringPiece result() const = 0;
};

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_ENV_H
