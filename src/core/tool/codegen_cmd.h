//
// Created by Arseny Tolmachev on 2018/06/13.
//

#ifndef JUMANPP_T9_CODEGEN_CMD_H
#define JUMANPP_T9_CODEGEN_CMD_H

#include "util/status.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace tool {

Status generateStaticFeatures(StringPiece specFile, StringPiece baseName,
                              StringPiece className);

}  // namespace tool
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_T9_CODEGEN_CMD_H
