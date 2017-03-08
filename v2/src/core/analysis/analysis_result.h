//
// Created by Arseny Tolmachev on 2017/03/08.
//

#ifndef JUMANPP_ANALYSIS_RESULT_H
#define JUMANPP_ANALYSIS_RESULT_H


#include "lattice_config.h"
#include "util/array_slice.h"

namespace jumanpp {
namespace core {
namespace analysis {

class Lattice;

class AnalysisNode {
  Lattice *lattice_;
  util::ArraySlice<ConnectionBeamElement> beam_;
public:


};

} // analysis
} // core
} // jumanpp

#endif //JUMANPP_ANALYSIS_RESULT_H
