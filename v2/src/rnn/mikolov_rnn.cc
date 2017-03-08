//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "mikolov_rnn.h"

namespace jumanpp {
namespace rnn {
namespace mikolov {

struct JPP_PACKED MikolovRnnModePackedlHeader {
  u64 sizeVersion;
  u64 maxEntTableSize;
  u32 maxentOrder;
  bool useNce;
  float nceLnz;
  bool reversedSentence;
  char layerType[LayerNameMaxSize];
  u32 layerCount;
  u32 hsArity;  
};

Status readHeader(StringPiece data, MikolovRnnModelHeader *header) {  
  auto packed = reinterpret_cast<const MikolovRnnModePackedlHeader*>(data.begin());  
  auto sv = packed->sizeVersion;
  auto vers = sv / VersionStepSize;
  if (vers != 6) {
    return Status::InvalidParameter() << "invalid rnn model version " << vers << " can handle only 6";
  }
  
  if (!packed->useNce) {
    return Status::InvalidParameter() << "model was trained without nce, we support only nce models";
  }

  auto piece = StringPiece{packed->layerType};
  if (piece != "sigmoid") {
    return Status::InvalidParameter() << "only sigmoid activation is supported, model had " << piece;
  }

  header->layerSize = (u32)(packed->sizeVersion % VersionStepSize);
  header->nceLnz = packed->nceLnz;
  header->maxentOrder = packed->maxentOrder;
  header->maxentSize = packed->maxEntTableSize;

  return Status::Ok();
}


}
}
}