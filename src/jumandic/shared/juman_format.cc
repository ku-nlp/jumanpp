//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "juman_format.h"
#include "core/analysis/charlattice.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status JumanFormat::format(const core::analysis::Analyzer& analysis,
                           StringPiece comment) {
  printer.reset();
  JPP_RETURN_IF_ERROR(analysisResult.reset(analysis));
  JPP_RETURN_IF_ERROR(analysisResult.fillTop1(&top1));

  auto& outMgr = analysis.output();

  if (!comment.empty()) {
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

namespace {
StringPiece escapeForJumanOutput(StringPiece in) {
  if (in.size() == 1) {
    switch (in[0]) {
      // return fullwidth char
      case '\t':
        return StringPiece("\\t");
      case ' ':
        return StringPiece("　");
      case '"':
        return StringPiece("”");
      case '<':
        return StringPiece("＜");
      case '>':
        return StringPiece("＞");
    }
  }
  return in;
}
}  // namespace

void formatNormalizedFeature(util::io::FastPrinter& p, i32 featureVal) {
  p << "非標準表記:";
  using m = core::analysis::charlattice::Modifiers;
  namespace c = core::analysis::charlattice;

  auto flag = static_cast<c::Modifiers>(featureVal);

  if (c::ExistFlag(flag, m::REPLACE)) {
    p << "R";
  }
  if (c::ExistFlag(flag, m::REPLACE_SMALLKANA)) {
    p << "s";
  }
  if (c::ExistFlag(flag, m::REPLACE_PROLONG)) {
    p << "p";
  }
  if (c::ExistFlag(flag, m::REPLACE_EROW_WITH_E)) {
    p << "e";
  }
  if (c::ExistFlag(flag, m::DELETE)) {
    p << "D";
  }
  if (c::ExistFlag(flag, m::DELETE_PROLONG)) {
    p << "P";
  }
  if (c::ExistFlag(flag, m::DELETE_SMALLKANA)) {
    p << "S";
  }
  if (c::ExistFlag(flag, m::DELETE_HASTSUON)) {
    p << "H";
  }
  if (c::ExistFlag(flag, m::DELETE_LAST)) {
    p << "L";
  }
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

    auto fieldBuffer = walker.features();

    JumandicPosId rawId{fieldBuffer[1], fieldBuffer[2],
                        fieldBuffer[4],  // conjForm and conjType are reversed
                        fieldBuffer[3]};

    auto newId = idResolver.dicToJuman(rawId);

    printer << escapeForJumanOutput(flds.surface[walker]) << " ";
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

    auto eptr = walker.eptr();
    bool hasFeatures = eptr.isSpecial() || res.hasNext() || !canonic.empty();

    if (!hasFeatures) {
      printer << "NIL";
    } else {
      bool output = false;
      printer << '"';
      if (!canonic.empty()) {
        printer << "代表表記:" << canonic;
        if (res.hasNext()) {
          printer << " ";
        }
        output = true;
      }
      while (res.next()) {
        output = true;
        printer << res.key();
        if (res.hasValue()) {
          printer << ':' << res.value();
        }
        if (res.hasNext()) {
          printer << " ";
        }
      }

      if (eptr.isSpecial()) {
        auto ufld =
            om.valueOfUnkPlaceholder(eptr, jumandic::NormalizedPlaceholderIdx);
        if (ufld != 0) {
          if (output) {
            printer << " ";
          }
          formatNormalizedFeature(printer, ufld);
        }
      }

      printer << '"';
    }

    printer << "\n";
    first = false;
  }
  return true;
}

JumanFormat::JumanFormat() : analysisResult() {
  printer.reserve(16 * 1024);  // 16k
}

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp