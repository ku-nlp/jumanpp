//
// Created by nakao on 2017/05/24.
//

#include "onomatopoeia_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

OnomatopoeiaUnkMaker::OnomatopoeiaUnkMaker(
    const dic::DictionaryEntries& entries_, chars::CharacterClass charClass_,
    UnkNodeConfig&& info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {}

bool OnomatopoeiaUnkMaker::spawnNodes(const AnalysisInput& input,
                                      UnkNodesContext* ctx,
                                      LatticeBuilder* lattice) const {
  using dic::TraverseStatus;
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto trav = entries_.traversal();
    LatticePosition nextstep = i;
    TraverseStatus status;
    auto pattern = FindOnomatopoeia(codepoints, i);
    for (LatticePosition halfLen = 2; halfLen * 2 <= MaxOnomatopoeiaLength;
         ++halfLen) {
      if ((pattern & HalfLenToPattern(halfLen)) != Pattern::None) {
        for (; nextstep < i + halfLen * 2; ++nextstep) {
          status = trav.step(codepoints[nextstep].bytes);
        }
        switch (status) {
          case TraverseStatus::NoNode: {
            LatticePosition start = i;
            LatticePosition end = i + halfLen * LatticePosition(2);
            auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
            lattice->appendSeed(ptr, start, end);
            break;
          }
          case TraverseStatus::NoLeaf: {
            LatticePosition start = i;
            LatticePosition end = i + halfLen * LatticePosition(2);
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
    const CodepointStorage& codepoints, LatticePosition start) const {
  if (start + MinOnomatopoeiaLength < codepoints.size()) {
    return Pattern::None;
  }
  JPP_DCHECK_IN(start, 0, codepoints.size());
  auto& cp1 = codepoints[start];
  if (!cp1.hasClass(charClass_)) {
    return Pattern::None;
  }
  auto cp1Class = cp1.charClass;
  JPP_DCHECK_IN(start + 1, 0, codepoints.size());
  if (!codepoints[start + 1].hasClass(cp1Class)) {
    return Pattern::None;
  }

  Pattern pattern = Pattern::None;
  for (LatticePosition halfLen = 2; halfLen * 2 <= MaxOnomatopoeiaLength &&
                                    start + halfLen * 2 - 1 < codepoints.size();
       ++halfLen) {
    JPP_DCHECK_IN(start + halfLen, 0, codepoints.size());
    auto& cp2 = codepoints[start + halfLen];
    if (!cp2.hasClass(cp1Class)) {
      // No more onomatopoeia starting with cp1.
      return pattern;
    }

    if (cp1.codepoint != cp2.codepoint) {
      continue;
    }

    bool isOnomatopoeia = true;
    LatticePosition p = 1;
    for (; p < halfLen; ++p) {
      if (codepoints[start + p].codepoint !=
          codepoints[start + halfLen + p].codepoint) {
        isOnomatopoeia = false;
        break;
      }
    }
    if (isOnomatopoeia) {
      pattern = pattern | HalfLenToPattern(halfLen);
    }
  }
  return pattern;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp