//
// Created by Arseny Tolmachev on 2017/03/08.
//

#ifndef JUMANPP_MIKOLOV_RNN_H
#define JUMANPP_MIKOLOV_RNN_H

#include "util/types.hpp"
#include "util/status.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace rnn {
namespace mikolov {

constexpr static size_t LayerNameMaxSize = 64;
constexpr static u64 VersionStepSize = 10000;

#define JPP_PACKED __attribute__((__packed__))

struct MikolovRnnModelHeader {
  u32 layerSize;
  u32 maxentOrder;
  u64 maxentSize;
  float nceLnz;

};

Status readHeader(StringPiece data, MikolovRnnModelHeader* header);

}



} // rnn
} // jumanpp

#endif //JUMANPP_MIKOLOV_RNN_H
