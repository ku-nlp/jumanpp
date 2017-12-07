//
// Created by Arseny Tolmachev on 2017/12/07.
//

#include "segmented_format.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace output {

Status SegmentedFormat::format(const core::analysis::Analyzer &analyzer,
                               StringPiece comment) {
  printer_.reset();
  JPP_RETURN_IF_ERROR(top1_.fillIn(analyzer.impl()->lattice()));

  auto nw = mgr_->nodeWalker();
  analysis::ConnectionPtr ptr;
  while (top1_.nextBoundary()) {
    if (!top1_.nextNode(&ptr)) {
      return JPPS_INVALID_PARAMETER << "failed to find a node at " << ptr;
    }
    if (!mgr_->locate(ptr.latticeNodePtr(), &nw)) {
      return JPPS_INVALID_PARAMETER << "failed to find a node at " << ptr;
    }
    printer_ << surface_[nw];
    if (!top1_.isLastBoundary()) {
      printer_ << separator_;
    }
  }

  printer_ << '\n';

  return Status::Ok();
}

StringPiece SegmentedFormat::result() const { return printer_.result(); }

Status SegmentedFormat::initialize(const analysis::OutputManager &mgr,
                                   const core::CoreHolder &core,
                                   StringPiece separator) {
  auto idxColumn = core.spec().dictionary.indexColumn;
  auto &surfField = core.spec().dictionary.fields[idxColumn];
  JPP_RETURN_IF_ERROR(mgr.stringField(surfField.name, &surface_));
  separator.assignTo(separator_);
  mgr_ = &mgr;
  return Status::Ok();
}
}  // namespace output
}  // namespace core
}  // namespace jumanpp