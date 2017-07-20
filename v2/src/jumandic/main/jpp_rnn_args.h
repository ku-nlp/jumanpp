//
// Created by Arseny Tolmachev on 2017/07/18.
//

#ifndef JUMANPP_JPP_RNN_ARGS_H
#define JUMANPP_JPP_RNN_ARGS_H

#include "args.h"
#include "core/analysis/rnn_scorer.h"

namespace jumanpp {
class RnnArgs {
  const core::analysis::rnn::RnnInferenceConfig defaultConf{};
  args::Group rnnGrp{"RNN parameters"};
  args::ValueFlag<float> nceBias{
      rnnGrp,
      "VALUE",
      "RNN NCE bias, default: " + std::to_string(defaultConf.nceBias),
      {"rnn-nce-bias"},
      defaultConf.nceBias};

  args::ValueFlag<float> unkConstantTerm{
      rnnGrp,
      "VALUE",
      "RNN UNK constant penalty, default: " +
          std::to_string(defaultConf.unkConstantTerm),
      {"rnn-unk-constant"},
      defaultConf.unkConstantTerm};

  args::ValueFlag<float> unkLengthPenalty{
      rnnGrp,
      "VALUE",
      "RNN UNK length penalty, default: " +
          std::to_string(defaultConf.unkLengthPenalty),
      {"rnn-unk-length"},
      defaultConf.unkLengthPenalty};

  args::ValueFlag<float> perceptronWeight{
      rnnGrp,
      "VALUE",
      "Perceptron feature weight, default: " +
          std::to_string(defaultConf.perceptronWeight),
      {"feature-weight-perceptron"},
      defaultConf.perceptronWeight};

  args::ValueFlag<float> rnnWeight{
      rnnGrp,
      "VALUE",
      "RNN feature weight, default: " + std::to_string(defaultConf.rnnWeight),
      {"feature-weight-rnn"},
      defaultConf.rnnWeight};

  args::ValueFlag<std::string> rnnUnk{
      rnnGrp,
      "UNK_TOKEN",
      "RNN token for UNK symbols, default: JPP_UNK",
      {"rnn-unk"},
      "JPP_UNK"};

 public:
  explicit RnnArgs(args::ArgumentParser& parser) { parser.Add(rnnGrp); }

  core::analysis::rnn::RnnInferenceConfig config() {
    auto copy = defaultConf;
    copy.nceBias = nceBias.Get();
    copy.unkConstantTerm = unkConstantTerm.Get();
    copy.unkLengthPenalty = unkLengthPenalty.Get();
    copy.perceptronWeight = perceptronWeight.Get();
    copy.rnnWeight = rnnWeight.Get();
    copy.unkSymbol = rnnUnk.Get();
    return copy;
  }
};

}  // namespace jumanpp

#endif  // JUMANPP_JPP_RNN_ARGS_H
