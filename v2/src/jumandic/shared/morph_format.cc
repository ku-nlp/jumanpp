//
// Created by Arseny Tolmachev on 2017/05/24.
//

#include "morph_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status MorphFormat::initialize(const core::analysis::OutputManager& om) {
  JPP_RETURN_IF_ERROR(fields_.initialize(om));
  return Status::Ok();
}

Status MorphFormat::format(const core::analysis::Analyzer& analyzer,
                           StringPiece comment) {
  printer_.reset();
  JPP_RETURN_IF_ERROR(analysisResult_.reset(analyzer));
  JPP_RETURN_IF_ERROR(analysisResult_.fillTop1(&top1_));

  auto& f = fields_;

  auto& om = analyzer.output();

  while (top1_.nextBoundary()) {
    core::analysis::ConnectionPtr cptr;
    if (!top1_.nextNode(&cptr)) {
      return Status::InvalidState()
             << "there were no nodes at boundary: " << top1_.currentBoundary();
    }

    if (!om.locate(cptr.latticeNodePtr(), &walker)) {
      return Status::InvalidState()
             << "could not find a ready node: " << top1_.currentBoundary()
             << " " << cptr.right;
    }

    if (fmrp_) {
      printer_ << f.surface[walker] << '_';
      printer_ << f.reading[walker] << '_';
      printer_ << f.baseform[walker] << '_';
      printer_ << f.pos[walker] << '_';
      printer_ << f.subpos[walker] << '_';
      printer_ << f.conjType[walker] << '_';
      printer_ << f.conjForm[walker];
    } else {
      printer_ << f.baseform[walker] << '_';
      printer_ << f.pos[walker] << ':';
      printer_ << f.subpos[walker];
    }

    printer_ << ' ';
  }

  if (comment.size() > 0) {
    printer_ << "# " << comment;
  }
  return Status::Ok();
}

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp