//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_SPEC_SERIALIZATION_H
#define JUMANPP_SPEC_SERIALIZATION_H

#include "core/spec/spec_types.h"
#include "util/coded_io.h"

namespace jumanpp {
namespace core {
namespace spec {

void saveSpec(const AnalysisSpec& spec, util::CodedBuffer* buf);
bool loadSpec(StringPiece data, AnalysisSpec*result);

} // spec
} // core
} // jumanpp


#endif //JUMANPP_SPEC_SERIALIZATION_H
