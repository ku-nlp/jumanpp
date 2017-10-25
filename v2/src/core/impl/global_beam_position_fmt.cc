//
// Created by Arseny Tolmachev on 2017/10/25.
//

#include "global_beam_position_fmt.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace output {

Status GlobalBeamPositionFormat::initialize(
    const core::analysis::Analyzer& analyzer) {
  auto& dic = analyzer.output().dic();
  auto totalFlds = dic.fields().totalFields();
  for (int i = 0; i < totalFlds; ++i) {
    auto& fld = dic.fields().at(i);
    if (fld.isSurfaceField) {
      JPP_RETURN_IF_ERROR(analyzer.output().stringField(fld.name, &surface_));
      return Status::Ok();
    }
  }
  return JPPS_INVALID_PARAMETER << "dictionary did not contain a surface field";
}

Status GlobalBeamPositionFormat::format(
    const core::analysis::Analyzer& analyzer, StringPiece comment) {
  printer_.reset();
  if (!comment.empty()) {
    printer_ << "# " << comment << "\n";
  }

  auto lat = analyzer.impl()->lattice();
  auto eosBnd = lat->boundary(lat->createdBoundaryCount() - 1);
  auto top1Eos = eosBnd->starts()->beamData().at(0);

  auto proc = analysis::ScoreProcessor::make(analyzer.impl());
  JPP_RETURN_IF_ERROR(std::move(proc.first));
  auto sp = proc.second;

  const analysis::ConnectionPtr* ptr = &top1Eos.ptr;
  auto score = top1Eos.totalScore;

  auto& om = analyzer.output();
  auto walker = om.nodeWalker();

  while (ptr->previous->boundary >= 2) {
    if (!om.locate(ptr->previous->latticeNodePtr(), &walker)) {
      return JPPS_INVALID_STATE << "could not locate a node for output";
    }

    if (!walker.next()) {
      return JPPS_INVALID_STATE << "could not locate a node for output";
    }

    auto res = sp->makeGlobalBeam(ptr->boundary, maxElems_);

    i32 pos = 0;
    for (auto& el : res) {
      if (el.left() == ptr->left && el.beam() == ptr->beam) {
        printer_ << surface_[walker] << '\t' << pos << '\t' << score;
        break;
      }
      ++pos;
    }

    if (pos == res.size()) {
      printer_ << surface_[walker] << '\t' << '*' << '\t' << score;
    }

    if (pos != 0) {
      auto el = res[0];
      auto curEnd = lat->boundary(ptr->boundary)->ends();
      auto ptr2 = curEnd->nodePtrs().at(el.left());
      auto beg2 = lat->boundary(ptr2.boundary)->starts();
      auto beam = beg2->beamData().row(ptr2.position).at(el.beam());
      auto score = beam.totalScore;
      auto eptr = beg2->nodeInfo().at(ptr2.position).entryPtr();
      if (!om.locate(eptr, &walker)) {
        return JPPS_INVALID_STATE << "could not locate a node for output";
      }

      if (!walker.next()) {
        return JPPS_INVALID_STATE << "could not locate a node for output";
      }

      printer_ << '\t' << surface_[walker] << '\t' << score;
    }

    printer_ << '\n';

    auto beamPos = ptr->beam;
    ptr = ptr->previous;
    auto s = lat->boundary(ptr->boundary)->starts();
    score = s->beamData().row(ptr->right).at(beamPos).totalScore;
  }

  printer_ << '\n';

  return Status::Ok();
}

}  // namespace output
}  // namespace core
}  // namespace jumanpp