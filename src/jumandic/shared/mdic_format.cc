//
// Created by Arseny Tolmachev on 2017/07/20.
//

#include "mdic_format.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status MdicFormat::format(const core::analysis::Analyzer& analyzer,
                          StringPiece comment) {
  printer_.reset();

  auto& om = analyzer.output();
  auto ai = analyzer.impl();

  displayed_.clear();
  auto lat = ai->lattice();

  for (int bidx = 2; bidx < lat->createdBoundaryCount() - 1; ++bidx) {
    auto bnd = lat->boundary(bidx);
    auto ninfo = bnd->starts()->nodeInfo();

    for (int i = 0; i < ninfo.size(); ++i) {
      auto eptr = ninfo.at(i).entryPtr();
      if (eptr.isDic() && displayed_.count(eptr.rawValue()) == 0) {
        JPP_RETURN_IF_ERROR(formatNode(eptr.rawValue(), om));
      }
    }
  }
  return Status::Ok();
}

StringPiece MdicFormat::result() const { return printer_.result(); }

Status MdicFormat::initialize(const core::analysis::OutputManager& om) {
  return flds_.initialize(om);
}

namespace fmt {

util::io::FastPrinter& printMaybeQuoted(util::io::FastPrinter& printer,
                                        StringPiece data) {
  auto end = data.end();
  auto beg = data.begin();
  auto noCommas = std::find(beg, end, ',') == end;
  auto it = std::find(beg, end, '"');
  if (noCommas && it == end) {
    printer << data;
  } else {
    printer << '"';
    while (it != end) {
      printer << StringPiece{beg, it};
      if (it != end) {
        printer << '"';
      }
      beg = it;
      it = std::find(it + 1, end, '"');
    }
    printer << StringPiece{beg, end};
    printer << '"';
  }
  return printer;
}

util::io::FastPrinter& printMaybeQuoteStringList(
    util::io::FastPrinter& printer, core::analysis::KVListIterator items) {
  auto shouldQuote = false;
  auto itemsCopy = items;
  while (itemsCopy.next()) {
    auto sp = itemsCopy.key();
    auto it = std::find_if(sp.begin(), sp.end(),
                           [](char c) { return c == '"' || c == ','; });
    if (it != sp.end()) {
      shouldQuote = true;
      break;
    }
    if (!itemsCopy.hasValue()) {
      continue;
    }
    sp = itemsCopy.value();
    it = std::find_if(sp.begin(), sp.end(),
                      [](char c) { return c == '"' || c == ','; });
    if (it != sp.end()) {
      shouldQuote = true;
      break;
    }
  }

  auto printQuoted = [&](StringPiece sp) {
    auto beg = sp.begin();
    auto it = std::find(beg, sp.end(), '"');
    while (it != sp.end()) {
      printer << StringPiece{beg, it};
      printer << '"';
      beg = it;
      it = std::find(it + 1, sp.end(), '"');
    }
    printer << StringPiece{beg, sp.end()};
  };

  if (shouldQuote) {
    printer << '"';
    while (items.next()) {
      printQuoted(items.key());
      if (items.hasValue()) {
        printer << ':';
        printQuoted(items.value());
      }
      if (items.hasNext()) {
        printer << ' ';
      }
    }
    printer << '"';
  } else {
    while (items.next()) {
      printer << items.key();
      if (items.hasValue()) {
        printer << ':' << items.value();
      }
      if (items.hasNext()) {
        printer << ' ';
      }
    }
  }

  return printer;
}

}  // namespace fmt

using namespace jumanpp::jumandic::output::fmt;

Status MdicFormat::formatNode(i32 nodeRawPtr,
                              const core::analysis::OutputManager& om) {
  if (!om.locate(core::EntryPtr{nodeRawPtr}, &walker)) {
    return Status::InvalidState()
           << "could not find a ready node with eptr: " << nodeRawPtr;
  }

  auto& f = flds_;

  while (walker.next()) {
    printMaybeQuoted(printer_, f.surface[walker]);
    printer_ << ",0,0,0,";
    printMaybeQuoted(printer_, f.pos[walker]) << ',';
    printMaybeQuoted(printer_, f.subpos[walker]) << ',';
    printMaybeQuoted(printer_, f.conjForm[walker]) << ',';
    printMaybeQuoted(printer_, f.conjType[walker]) << ',';
    printMaybeQuoted(printer_, f.baseform[walker]) << ',';
    printMaybeQuoted(printer_, f.reading[walker]) << ',';
    printMaybeQuoted(printer_, f.canonicForm[walker]) << ',';

    printMaybeQuoteStringList(printer_, f.features[walker]);

    printer_ << '\n';
  }

  return Status::Ok();
}

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp