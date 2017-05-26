//
// Created by nakao on 2017/05/24.
//

#include "onomatopoeia_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

OnomatopoeiaUnkMaker::OnomatopoeiaUnkMaker(const dic::DictionaryEntries& entries_,
                                           chars::CharacterClass charClass_,
                                           UnkNodeConfig&& info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {}

bool OnomatopoeiaUnkMaker::spawnNodes(const AnalysisInput& input,
                                      UnkNodesContext* ctx,
                                      LatticeBuilder* lattice) const {
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto trav = entries_.traversal();
    LatticePosition nextstep = i;
    jumanpp::core::TraverseStatus status;
    auto pattern = FindOnomatopoeia(codepoints, i);
    for (LatticePosition len = 2; len * 2 <= MaxOnomatopoeiaLength; ++len) {
      if ((pattern & Pattern(1 << len)) != Pattern::None) {
        for (;nextstep < i + len * 2; ++nextstep) {
          status = trav.step(codepoints[nextstep].bytes);
        }
        switch (status) {
          case TraverseStatus::NoNode: {
            LatticePosition start = i;
            LatticePosition end = i + len * LatticePosition(2);
            auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
            lattice->appendSeed(ptr, start, end);
            break;
          }
          case TraverseStatus::NoLeaf: {
            LatticePosition start = i;
            LatticePosition end = i + len * LatticePosition(2);
            auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
            lattice->appendSeed(ptr, start, end);
            break;
          }
          case TraverseStatus::Ok:
            continue;
        }
      }
    }
  }
  return true;
}

OnomatopoeiaUnkMaker::Pattern OnomatopoeiaUnkMaker::FindOnomatopoeia(
    const CodepointStorage & codepoints, LatticePosition start) const {
  auto& cp1 = codepoints[start];
  if (!cp1.hasClass(charClass_)) {
    return Pattern::None;
  }
  auto cp1Class = cp1.charClass;
  if (!codepoints[start+1].hasClass(cp1Class)) {
    return Pattern::None;
  }

  Pattern pattern = Pattern::None;
  for (LatticePosition len = 2; len * 2 <= MaxOnomatopoeiaLength &&
       start + len * 2 - 1 < codepoints.size(); ++len) {
    auto& cp2 = codepoints[start+len];
    if (!cp2.hasClass(cp1Class)) {
      // No more onomatopoeia starting with cp1.
      return pattern;
    }

    if (cp1.codepoint != cp2.codepoint) {
      continue;
    }

    bool isOnomatopoeia = true;
    LatticePosition p = 1;
    for (; p < len; ++p) {
      if (codepoints[start+p].codepoint != codepoints[start+len+p].codepoint) {
        isOnomatopoeia = false;
        break;
      }
    }
    if (isOnomatopoeia) {
      pattern = pattern | Pattern(1 << len) & Pattern::All;
    }
  }
  return pattern;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp