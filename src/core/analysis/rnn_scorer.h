//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_RNN_SCORER_H
#define JUMANPP_RNN_SCORER_H

#include <ostream>
#include <string>
#include <vector>
#include "util/cfg.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

struct RnnInferenceConfig {
  util::Cfg<float> nceBias = 0;
  util::Cfg<float> unkConstantTerm = -6.0f;
  util::Cfg<float> unkLengthPenalty = -1.5f;
  util::Cfg<float> perceptronWeight = 1.0f;
  util::Cfg<float> rnnWeight = 1.0f;
  util::Cfg<std::string> eosSymbol{"</s>"};
  util::Cfg<std::string> unkSymbol{"<unk>"};
  util::Cfg<std::vector<std::string>> rnnFields;
  util::Cfg<std::string> fieldSeparator{","};

  bool operator==(const RnnInferenceConfig& other) const;
  bool isDefault() const;
  void mergeWith(const RnnInferenceConfig& o) {
    nceBias.mergeWith(o.nceBias);
    unkConstantTerm.mergeWith(o.unkConstantTerm);
    unkLengthPenalty.mergeWith(o.unkLengthPenalty);
    perceptronWeight.mergeWith(o.perceptronWeight);
    rnnWeight.mergeWith(o.rnnWeight);
    eosSymbol.mergeWith(o.eosSymbol);
    unkSymbol.mergeWith(o.unkSymbol);
    rnnFields.mergeWith(o.rnnFields);
    fieldSeparator.mergeWith(o.fieldSeparator);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const RnnInferenceConfig &config);
};

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_SCORER_H
