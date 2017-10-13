//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "analysis_result.h"
#include <cmath>
#include <util/status.hpp>
#include "analyzer.h"
#include "analyzer_impl.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

jumanpp::Status AnalysisResult::reset(const Analyzer &analyzer) {
  this->ptr_ = analyzer.impl()->lattice();
  return Status::Ok();
}

Status AnalysisResult::fillTop1(AnalysisPath *result) {
  return result->fillIn(ptr_);
}

Status AnalysisPath::fillIn(Lattice *l) {
  auto bnds = l->createdBoundaryCount();
  JPP_DCHECK_GE(bnds, 3);
  elems_.clear();
  offsets_.clear();
  elems_.reserve(bnds * 2);
  offsets_.reserve(bnds);
  if (bnds == 3) {  // empty string
    return Status::Ok();
  }

  auto lastOne = l->boundary(bnds - 1);
  auto lastStart = lastOne->starts();

  if (lastStart->arraySize() != 1) {
    return JPPS_INVALID_STATE << "last boundary had more than one node!";
  }

  if (lastStart->nodeInfo().at(0).entryPtr() != EntryPtr::EOS()) {
    return JPPS_INVALID_STATE << "last node was not EOS";
  }

  const ConnectionPtr *topPtr = &lastStart->beamData().at(0).ptr;
  i32 myBeam = 0;

  // 0 and 1 are BOS
  offsets_.push_back(0);
  while (topPtr->boundary > 2) {
    auto bnd = l->boundary(topPtr->boundary);
    auto starts = bnd->starts();
    auto beamAtBnd = starts->beamData().row(topPtr->right);

    auto topItem = beamAtBnd.at(myBeam);
    elems_.push_back(*topItem.ptr.previous);
    for (i32 id = myBeam + 1; id < beamAtBnd.size(); ++id) {
      auto nextItem = beamAtBnd.at(id);
      JPP_DCHECK_LE(nextItem.totalScore, topItem.totalScore);
      if (nextItem.totalScore < topItem.totalScore) {
        break;
      }
      elems_.push_back(*nextItem.ptr.previous);
    }
    offsets_.push_back((u32)elems_.size());
    myBeam = topPtr->beam;
    topPtr = topPtr->previous;
  }

  currentChunk_ = -1;

  return Status::Ok();
}
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp