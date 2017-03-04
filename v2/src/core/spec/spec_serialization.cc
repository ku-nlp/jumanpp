//
// Created by Arseny Tolmachev on 2017/03/04.
//

#include "spec_serialization.h"
#include "spec_ser.h"

namespace jumanpp {
namespace core {
namespace spec {

void saveSpec(const AnalysisSpec& spec, util::CodedBuffer* buf) {
  util::serialization::Saver s{buf};
  s.save(spec);
}

bool loadSpec(StringPiece data, AnalysisSpec* result) {
  util::serialization::Loader l{data};
  return l.load(result);
}

}  // spec
}  // core
}  // jumanpp