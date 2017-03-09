//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "juman_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status JumanFormat::format(const core::analysis::Analyzer &analysis) {
  printer.reset();
  JPP_RETURN_IF_ERROR(analysisResult.reset(analysis));
  JPP_RETURN_IF_ERROR(analysisResult.fillTop1(&top1));

  auto& outMgr = analysis.output();

  while (top1.nextBoundary()) {
    if(top1.remainingNodesInChunk() <= 0) {
      return Status::InvalidState() << "no nodes in chunk";
    }
    core::analysis::ConnectionPtr connPtr;
    if (!top1.nextNode(&connPtr)) {
      return Status::InvalidState() << "failed to load a node";
    }

    formatOne(outMgr, connPtr, true);
    while (top1.nextNode(&connPtr)) {
      formatOne(outMgr, connPtr, false);
    }
  }
  printer << "EOS\n";

  return Status::Ok();
}

inline const StringPiece& ifEmpty(const StringPiece& s1, const StringPiece& s2) {
  if (s1.size() > 0) {
    return s1;
  }
  return s2;
}

bool JumanFormat::formatOne(const core::analysis::OutputManager& om, const core::analysis::ConnectionPtr &ptr, bool first) {
  core::analysis::LatticeNodePtr nodePtr {ptr.boundary, ptr.right};
  JPP_RET_CHECK(om.locate(nodePtr, &walker));
  while (walker.next()) {
    if (!first) {
      printer << "@ ";
    }
    printer << flds.surface[walker] << " ";
    printer << flds.baseform[walker] << " ";
    printer << flds.reading[walker] << " ";
    printer << ifEmpty(flds.pos[walker], "*") << " " << 0 << " ";
    printer << ifEmpty(flds.subpos[walker], "*") << " " << 0 << " ";
    printer << ifEmpty(flds.conjType[walker], "*") << " " << 0 << " ";
    printer << ifEmpty(flds.conjForm[walker], "*") << " " << 0 << " ";
    auto res = flds.features[walker];
    auto canonic = flds.canonicForm[walker];
    if (res.isEmpty()) {
      if (canonic.size() == 0) {
        printer << "NIL";
      } else {
        printer << "\"代表表記:" << canonic << "\"";
      }
    } else {
      printer << '"';
      if (canonic.size() > 0) {
        printer << "代表表記:" << canonic;
        if (!res.isEmpty()) {
          printer << " ";
        }
      }
      StringPiece data;
      while (res.next(&data)) {
        printer << data;
        if (!res.isEmpty()) {
          printer << " ";
        }
      }
      printer << '"';
    }
    printer << "\n";
    first = false;
  }
  return true;
}

} // output
} // jumandic
} // jumanpp