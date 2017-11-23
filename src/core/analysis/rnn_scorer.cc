//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "rnn_scorer.h"
#include "util/debug_output.h"

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

std::ostream &operator<<(std::ostream &os, const RnnInferenceConfig &config) {
  os << "\n~~~RNN CONFIG~~~\nnceBias: " << config.nceBias
     << "\nunkConstantTerm: " << config.unkConstantTerm
     << "\nunkLengthPenalty: " << config.unkLengthPenalty
     << "\nperceptronWeight: " << config.perceptronWeight
     << "\nrnnWeight: " << config.rnnWeight
     << "\neosSymbol: " << config.eosSymbol
     << "\nunkSymbol: " << config.unkSymbol
     << "\nrnnFields: " << VOut(config.rnnFields.value())
     << "\nfieldSeparator: " << config.fieldSeparator
     << "\n~~~RNN CONFIG END~~~";
  return os;
}

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
