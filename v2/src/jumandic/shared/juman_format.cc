//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "juman_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status JumanFormat::format(const core::analysis::Analyzer& analysis,
                           StringPiece comment) {
  printer.reset();
  JPP_RETURN_IF_ERROR(analysisResult.reset(analysis));
  JPP_RETURN_IF_ERROR(analysisResult.fillTop1(&top1));

  auto& outMgr = analysis.output();

  if (comment.size() > 0) {
    printer << "# " << comment << '\n';
  }

  while (top1.nextBoundary()) {
    if (top1.remainingNodesInChunk() <= 0) {
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

inline const StringPiece& ifEmpty(const StringPiece& s1,
                                  const StringPiece& s2) {
  if (s1.size() > 0) {
    return s1;
  }
  return s2;
}

bool JumanFormat::formatOne(const core::analysis::OutputManager& om,
                            const core::analysis::ConnectionPtr& ptr,
                            bool first) {
  core::analysis::LatticeNodePtr nodePtr{ptr.boundary, ptr.right};
  JPP_RET_CHECK(om.locate(nodePtr, &walker));
  while (walker.next()) {
    if (!first) {
      printer << "@ ";
    }

    JumandicPosId rawId{fieldBuffer[2], fieldBuffer[3],
                        fieldBuffer[5],  // conjForm and conjType are reversed
                        fieldBuffer[4]};

    auto newId = idResolver.dicToJuman(rawId);

    printer << flds.surface[walker] << " ";
    printer << flds.reading[walker] << " ";
    printer << flds.baseform[walker] << " ";
    printer << ifEmpty(flds.pos[walker], "*") << " " << newId.pos << " ";
    printer << ifEmpty(flds.subpos[walker], "*") << " " << newId.subpos << " ";
    printer << ifEmpty(flds.conjType[walker], "*") << " " << newId.conjType
            << " ";
    printer << ifEmpty(flds.conjForm[walker], "*") << " " << newId.conjForm
            << " ";
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

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp