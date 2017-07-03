//
// Created by Arseny Tolmachev on 2017/07/03.
//

#ifndef JUMANPP_RNN_SERIALIZATION_H
#define JUMANPP_RNN_SERIALIZATION_H

#include "util/serialization.h"
#include "rnn_scorer.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

struct RnnSerializedData {
  jumanpp::rnn::mikolov::MikolovRnnModelHeader modelHeader;
  jumanpp::core::analysis::rnn::RnnInferenceConfig config;
  u32 targetIdx_;
};

template <typename Arch>
void Serialize(Arch& a, RnnSerializedData& data) {
  a & data.modelHeader.layerSize;
  a & data.modelHeader.maxentOrder;
  a & data.modelHeader.maxentSize;
  a & data.modelHeader.vocabSize;
  a & data.modelHeader.nceLnz;
  a & data.config.unkConstantTerm;
  a & data.config.unkLengthPenalty;
  a & data.config.nceBias;
}

} // rnn
} // analysis
} // core
} // jumanpp

#endif //JUMANPP_RNN_SERIALIZATION_H
