//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_MODEL_FORMAT_H
#define JUMANPP_MODEL_FORMAT_H

#include <vector>
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace model {

enum class ModelPartKind { Dictionary, Perceprton, Rnn };

struct ModelPart {
  ModelPartKind kind;
  std::vector<StringPiece> data;
};

struct ModelInfo {
  u64 specHash;
  u64 runtimeHash;
  std::vector<ModelPart> parts;
};

struct BlockPtr {
  u64 offset;
  u64 size;
};

struct ModelPartRaw {
  ModelPartKind kind;
  std::vector<BlockPtr> data;
};

struct ModelInfoRaw {
  u64 specHash;
  u64 runtimeHash;
  std::vector<ModelPartRaw> parts;
};

}  // model
}  // core
}  // jumanpp

#endif  // JUMANPP_MODEL_FORMAT_H
