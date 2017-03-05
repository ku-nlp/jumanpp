//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_JUMANDIC_SPEC_H
#define JUMANPP_JUMANDIC_SPEC_H

#include "core/spec/spec_types.h"

namespace jumanpp {
namespace jumandic {

class SpecFactory {
 public:
  static const StringPiece lexicalizedData;
  static Status makeSpec(core::spec::AnalysisSpec* spec);
};

}  // jumandic
}  // jumanpp

#endif  // JUMANPP_JUMANDIC_SPEC_H
