//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "rnn_scorer.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

bool RnnInferenceConfig::operator==(const RnnInferenceConfig &other) const {
  return nceBias == other.nceBias && unkConstantTerm == other.unkConstantTerm &&
         unkLengthPenalty == other.unkLengthPenalty &&
         perceptronWeight == other.perceptronWeight &&
         rnnWeight == other.rnnWeight;
}

bool RnnInferenceConfig::isDefault() const {
  return util::areAllDefault(nceBias, unkConstantTerm, unkLengthPenalty,
                             perceptronWeight, rnnWeight, eosSymbol, unkSymbol,
                             rnnFields, fieldSeparator);
}

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
