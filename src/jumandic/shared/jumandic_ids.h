//
// Created by Arseny Tolmachev on 2017/07/05.
//

#ifndef JUMANPP_JUMANDIC_IDS_H
#define JUMANPP_JUMANDIC_IDS_H

#include "util/common.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace jumandic {

struct JumandicTuple {
  StringPiece part1;
  StringPiece part2;
  i32 id1;
  i32 id2;
};

extern const JumandicTuple posInfo[];
extern const u32 posInfoCount;
extern const JumandicTuple conjInfo[];
extern const u32 conjInfoCount;

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_IDS_H
