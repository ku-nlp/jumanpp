//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_MODEL_FORMAT_SER_H
#define JUMANPP_MODEL_FORMAT_SER_H

#include "model_format.h"
#include "util/serialization.h"

namespace jumanpp {
namespace core {
namespace model {

SERIALIZE_ENUM_CLASS(ModelPartKind, int);

template <typename Arch>
void Serialize(Arch& a, BlockPtr& o) {
  a& o.offset;
  a& o.size;
}

template <typename Arch>
void Serialize(Arch& a, ModelPartRaw& o) {
  a& o.kind;
  a& o.data;
  a& o.start;
  a& o.end;
}

template <typename Arch>
void Serialize(Arch& a, ModelInfoRaw& o) {
  a& o.parts;
}

}  // namespace model
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_MODEL_FORMAT_SER_H
