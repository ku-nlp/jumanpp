//
// Created by Arseny Tolmachev on 2018/06/08.
//

#ifndef JUMANPP_SPEC_PARSER_H
#define JUMANPP_SPEC_PARSER_H

#include "spec_types.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace spec {

Status parseFromFile(StringPiece name, AnalysisSpec* result);

}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_PARSER_H
