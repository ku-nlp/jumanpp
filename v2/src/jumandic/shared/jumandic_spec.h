//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_JUMANDIC_SPEC_H
#define JUMANPP_JUMANDIC_SPEC_H

#include <core/spec/spec_dsl.h>
#include "core/spec/spec_types.h"

namespace jumanpp {
namespace jumandic {

static constexpr i32 JumandicNumFields = 9;

class SpecFactory {
 public:
  static const StringPiece lexicalizedData;
  static void fillSpec(core::spec::dsl::ModelSpecBuilder& bldr);
  static Status makeSpec(core::spec::AnalysisSpec* spec);
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_SPEC_H
