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

class AnalysisPath {
  std::vector<ConnectionBeamElement> beam_;
  std::vector<u32> sizes_;
  i32 currentChunk_;
  i32 currentNode_;
public:

  bool nextChunk();
  i32 remainingNodesInChunk() const;
  i32 totalNodesInChunk() const;
  bool nextNode(ConnectionBeamElement* result);


  Status fillIn(Lattice* l);
};

class Analyzer;

class AnalysisResult {
  Lattice* ptr_;
public:
  Status reset(const Analyzer& analyzer);
  Status fillTop1(AnalysisPath* result);
};

} // analysis
} // core
} // jumanpp

#endif //JUMANPP_ANALYSIS_RESULT_H
