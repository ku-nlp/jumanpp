//
// Created by Arseny Tolmachev on 2017/07/18.
//

#ifndef JUMANPP_JPP_RNN_ARGS_H
#define JUMANPP_JPP_RNN_ARGS_H

#include <args.h>
#include "core/analysis/rnn_scorer.h"

namespace jumanpp {
class RnnArgs {
  const core::analysis::rnn::RnnInferenceConfig defaultConf{};

  args::Group rnnGrp{"RNN parameters"};
  args::ValueFlag<float> nceBias{rnnGrp,
                                 "VALUE",
                                 "RNN NCE bias, default: from RNN model",
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
      "RNN token for UNK symbols, default: <unk>",
      {"rnn-unk"},
      defaultConf.unkSymbol};

  args::ValueFlag<std::string> rnnEos{rnnGrp,
                                      "EOS_TOKEN",
                                      "RNN token for EOS symbol, default: </s>",
                                      {"rnn-eos"}};

  args::ValueFlag<std::string> rnnFields{
      rnnGrp,
      "FLD1,FLD2,...",
      "Dic fields which are used in RNN, comma-separated",
      {"rnn-fields"}};

  args::ValueFlag<std::string> rnnFieldSeparator{
      rnnGrp,
      "SEP",
      "Separator for field values in RNN dictionary (default _)",
      {"rnn-separator"}};

 public:
  explicit RnnArgs(args::Group& parent) { parent.Add(rnnGrp); }

  core::analysis::rnn::RnnInferenceConfig config() {
    auto copy = defaultConf;
    copy.nceBias.set(nceBias);
    copy.unkConstantTerm.set(unkConstantTerm);
    copy.unkLengthPenalty.set(unkLengthPenalty);
    copy.perceptronWeight.set(perceptronWeight);
    copy.rnnWeight.set(rnnWeight);
    copy.unkSymbol.set(rnnUnk);
    copy.eosSymbol.set(rnnEos);
    copy.fieldSeparator.set(rnnFieldSeparator);
    if (rnnFields) {
      std::vector<std::string> values;
      auto& data = rnnFields.Get();
      auto start = data.begin();
      auto end = data.end();
      auto it = std::find(start, end, ',');
      while (it != end) {
        values.emplace_back(start, it);
        start = it;
        ++start;
        it = std::find(start, end, ',');
      }
      values.emplace_back(start, it);
      copy.rnnFields = std::move(values);
    }
    return copy;
  }
};

}  // namespace jumanpp

#endif  // JUMANPP_JPP_RNN_ARGS_H
