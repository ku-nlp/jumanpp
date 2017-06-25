//
// Created by morita on 2017/06/12.
//

#include "numeric_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

NumericUnkMaker::NumericUnkMaker(const dic::DictionaryEntries &entries_,
                                 chars::CharacterClass charClass_,
                                 UnkNodeConfig &&info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {
  for (std::string defPattern : prefixPatternsDef) {
    CodepointStorage pattern;
    preprocessRawData(defPattern, &pattern);
    prefixPatterns.emplace_back(pattern);
  }
  for (std::string defPattern : interfixPatternsDef) {
    CodepointStorage pattern;
    preprocessRawData(defPattern, &pattern);
    interfixPatterns.emplace_back(pattern);
  }
  for (std::string defPattern : suffixPatternsDef) {
    CodepointStorage pattern;
    preprocessRawData(defPattern, &pattern);
    suffixPatterns.emplace_back(pattern);
  }
}

bool NumericUnkMaker::spawnNodes(const AnalysisInput &input,
                                 UnkNodesContext *ctx,
                                 LatticeBuilder *lattice) const {
  // Spawn the longest matting node
  auto &codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto trav = entries_.traversal();
    LatticePosition nextstep = i;
    TraverseStatus status;

    auto length = FindLongestNumber(codepoints, i);  // 長さを直接返す
    if (length > 0) {
      for (; nextstep < i + length; ++nextstep) {
        status = trav.step(codepoints[nextstep].bytes);
      }

      switch (status) {
        case TraverseStatus::NoNode: {  // 同一表層のノード無し prefix でもない
          LatticePosition start = i;
          LatticePosition end = i + length;
          auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
          lattice->appendSeed(ptr, start, end);
        }
        case TraverseStatus::NoLeaf: {  // 先端
          LatticePosition start = i;
          LatticePosition end = i + length;
          auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
          lattice->appendSeed(ptr, start, end);
        }
        case TraverseStatus::Ok:
          // オノマトペはともかく，数詞は必ずつくるのでここで作る必要がある．
          LatticePosition start = i;
          LatticePosition end = i + length;
          auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
          lattice->appendSeed(ptr, start, end);
      }
    }
  }
  return true;
}

size_t NumericUnkMaker::checkInterfix(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const {
  // In other words, check conditions for fractions ("５分の１" or "数ぶんの１")
  auto restLength = codepoints.size() - (start + pos);

  if (pos > 0) {  // 先頭ではない
    for (auto itemp : interfixPatterns) {
      if (  // Interfix follows a number.
          codepoints[start + pos - 1].hasClass(charClass_) &&
          // The length of rest codepoints is longer than the pattern length
          restLength > itemp.size() &&
          // A number follows interfix.
          codepoints[start + pos + itemp.size()].hasClass(charClass_)) {
        bool matchFlag = true;
        // The pattern matches the codepoints.
        for (u16 index = 0; index < itemp.size(); ++index) {
          if (codepoints[start + pos + index].codepoint != itemp[index].codepoint) {
            matchFlag = false;
            break;
          }
        }
        if (matchFlag) return itemp.size();
      }
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkSuffix(const CodepointStorage &codepoints,
                                    LatticePosition start,
                                    LatticePosition pos) const {
  // In other words, check conditions for units ("５キロ" or "数ミリ")
  auto restLength = codepoints.size() - (start + pos);

  if (pos > 0) {
    for (auto itemp : suffixPatterns) {
      if (  // Suffix follows a number.
          codepoints[start + pos - 1].hasClass(charClass_) &&
          // The pattern length does not exceed that of rest codepoints.
          restLength >= itemp.size()) {
        // The pattern matches the codepoints.
        bool matchFlag = true;
        for (u16 index = 0; index < itemp.size(); ++index) {
          if (codepoints[start + pos + index].codepoint != itemp[index].codepoint) {
            matchFlag = false;
            break;
          }
        }
        if (matchFlag) return itemp.size();
      }
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkPrefix(const CodepointStorage &codepoints,
                                    LatticePosition start,
                                    LatticePosition pos) const {
  // In other words, check conditions for units ("５キロ" or "数ミリ")
  auto restLength = codepoints.size() - (start + pos);

  for (auto itemp : prefixPatterns) {
    if (  // A digit follows prefix.
        codepoints[start + pos + itemp.size()].hasClass(DigitClass) &&
        // The length of rest codepoints is longer than the pattern length
        restLength > itemp.size()) {
      bool matchFlag = false;
      // The pattern matches the codepoints.
      for (u16 index = 0; index < itemp.size(); ++index) {
        if (codepoints[start + pos + index].codepoint != itemp[index].codepoint) {
          matchFlag = false;
          break;
        }
      }
      if (matchFlag) return itemp.size();
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkExceptionalCharsInFigure(
    const CodepointStorage &codepoints, LatticePosition start,
    LatticePosition pos) const {
  size_t charLength = 0;
  if ((charLength = checkPrefix(codepoints, start, pos)) > 0) return charLength;
  if ((charLength = checkInterfix(codepoints, start, pos)) > 0)
    return charLength;
  if ((charLength = checkSuffix(codepoints, start, pos)) > 0) return charLength;
  if ((charLength = checkComma(codepoints, start, pos)) > 0) return charLength;
  if ((charLength = checkPeriod(codepoints, start, pos)) > 0) return charLength;
  return 0;
}

size_t NumericUnkMaker::checkComma(const CodepointStorage &codepoints,
                                   LatticePosition start,
                                   LatticePosition pos) const {
  // check exists of comma separated number beggining at pos.
  u16 numContinuedFigure = 0;
  u16 posComma = (start + pos);
  static const u16 commaInterval = 3;

  if (!codepoints[posComma].hasClass(CommaClass)) return 0;

  for (numContinuedFigure = 0;
       numContinuedFigure <= commaInterval + 1 &&
       posComma + numContinuedFigure < codepoints.size();
       ++numContinuedFigure) {
    if (not codepoints[posComma + numContinuedFigure].hasClass(FigureClass)) {
      break;
    }
  }
  if (numContinuedFigure == 3)
    return 1;
  else
    return 0;
}

size_t NumericUnkMaker::checkPeriod(const CodepointStorage &codepoints,
                                    LatticePosition start,
                                    LatticePosition pos) const {
  // check exists of comma separated number beggining at pos.
  unsigned int numContinuedFigure = 0;
  unsigned int posComma = (start + pos);
  static const u16 commaInterval = 3;

  if (!codepoints[posComma].hasClass(PeriodClass)) return 0;

  if (numContinuedFigure == 3)
    return 1;
  else
    return 0;
}

size_t NumericUnkMaker::FindLongestNumber(const CodepointStorage &codepoints,
                                          LatticePosition start) const {
  size_t longestNumberLength = 0;
  LatticePosition pos = 0;
  for (pos = 0; pos <= MaxNumericLength && start + pos < codepoints.size();
       ++pos) {
    auto &cp = codepoints[start + pos];
    if (!cp.hasClass(charClass_)) {
      size_t charLength = 0;

      // Check Exception
      if ((charLength = checkExceptionalCharsInFigure(codepoints, start, pos)) >
          0) {
        pos += charLength;
      } else {
        return longestNumberLength;
      }
    }
  }

  return pos;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
