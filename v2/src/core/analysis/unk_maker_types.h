//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_UNK_MAKE_TYPES_H
#define JUMANPP_UNK_MAKE_TYPES_H

#include "core/spec/spec_types.h"
#include "util/array_slice.h"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

struct UnkMakerInfo {
  i32 index;
  StringPiece name;
  spec::UnkMakerType type;
  i32 patternPtr;
  chars::CharacterClass charClass;
  std::vector<spec::UnkMakerFeature> features;
  std::vector<i32> output;
};

struct UnkMakersInfo {
  i32 numPlaceholders;
  std::vector<UnkMakerInfo> makers;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_UNK_MAKE_TYPES_H
