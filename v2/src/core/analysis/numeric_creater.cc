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

    auto length = FindLongestNumber(codepoints, i); // 長さを直接返す
    if (length > 0) {
      for (; nextstep < i + length; ++nextstep) {
        status = trav.step(codepoints[nextstep].bytes);
      }

      switch (status) {
      case TraverseStatus::NoNode: { // 同一表層のノード無し prefix でもない
        LatticePosition start = i;
        LatticePosition end = i + halfLen * LatticePosition(2);
        auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
        lattice->appendSeed(ptr, start, end);
      }
      case TraverseStatus::NoLeaf: { // 先端
        LatticePosition start = i;
        LatticePosition end = i + halfLen * LatticePosition(2);
        auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
        lattice->appendSeed(ptr, start, end);
      }
      case TraverseStatus::Ok:
        // オノマトペはともかく，数詞は必ずつくるのでここで作る必要がある．
        LatticePosition start = i;
        LatticePosition end = i + halfLen * LatticePosition(2);
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

  if (pos > 0) { // 先頭ではない
    for (auto itemp : interfixPatterns) {
      if ( // Interfix follows a number.
          codepoints[start + pos - 1].hasClass(charClass_) &&
          // The length of rest codepoints is longer than the pattern length
          restLength > itemp.size() &&
          // A number follows interfix.
          codepoints[start + pos + itemp.size()].hasClass(charClass_)) {
        bool matchFlag = true;
        // The pattern matches the codepoints.
        for (u16 index = 0; index < itemp.size(); ++index) {
          if (codepoints[start + pos + index] != itemp[index]) {
            match_flag = false;
            break;
          }
        }
        if (match_flag)
          return itemp.size();
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
      if ( // Suffix follows a number.
          codepoints[start + pos - 1].hasClass(charClass_) &&
          // The pattern length does not exceed that of rest codepoints.
          restLength >= itemp.size()) {
        // The pattern matches the codepoints.
        bool matchFlag = true;
        for (u16 index = 0; index < itemp.size(); ++index) {
          if (codepoints[start + pos + index] != itemp[index]) {
            match_flag = false;
            break;
          }
        }
        if (match_flag)
          return itemp.size();
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
    if ( // A digit follows prefix.
        codepoints[start + pos + itemp.size()].hasClass(DigitClass)
        // The length of rest codepoints is longer than the pattern length
        restLength > itemp.size()) {
      bool matchFlag = false;
      // The pattern matches the codepoints.
      for (u16 index = 0; index < itemp.size(); ++index) {
        if (codepoints[start + pos + index] != itemp[index]) {
          match_flag = false;
          break;
        }
      }
      if (match_flag)
        return itemp.size();
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkExceptionalCharsInFigure(
    const CodepointStorage &codepoints, LatticePosition start,
    LatticePosition pos) const {
  size_t charLength = 0;
  if ((charLength = checkPrefix(codepoints, start, pos)) > 0)
    return charLength;
  if ((charLength = checkInterfix(codepoints, start, pos)) > 0)
    return charLength;
  if ((charLength = checkSuffix(codepoints, start, pos)) > 0)
    return charLength;
  if ((charLength = checkComma(codepoints, start, pos)) > 0)
    return charLength;
  if ((charLength = checkPeriod(codepoints, start, pos)) > 0)
    return charLength;
  return 0;
}

size_t NumericUnkMaker::checkComma(const CodepointStorage &codepoints,
                                   LatticePosition start,
                                   LatticePosition pos) const {
  // check exists of comma separated number beggining at pos.
  unsigned int numContinuedFigure = 0;
  unsigned int posCamma = (start + pos);
  static const u16 commaInterval = 3;

  if (!codepoints[posCamma].hasClass(CammaClass))
    return 0;

  for (u16 i = 0; i <= commaInterval + 1 &&
                      posCamma + numContinuedFigure < codepoints.size();
       ++i) {
    if (codepoints[posCamma + numContinuedFigure].hasClass(FigureClass)) {
      ++numContinuedFigure;
    } else {
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
  unsigned int posCamma = (start + pos);
  static const u16 commaInterval = 3;

  if (!codepoints[posCamma].hasClass(PeriodClass))
    return 0;

  if (numContinuedFigure == 3)
    return 1;
  else
    return 0;
}

size_t NumericUnkMaker::FindLongestNumber(const CodepointStorage &codepoints,
                                          LatticePosition start) const {

  // charClass_ = Number, camma, specialpattern
  // FIGURE = 0x00000008,
  // MIDDLE_DOT = 0x00000020,  // ・
  // COMMA = 0x00000040,       //　，
  // KANJI_FIGURE = 0x00000800,  // KANJI+FIGURE にするべき？
  // FIGURE_EXCEPTION = 0x00040000,  // 数，何
  // FIGURE_DIGIT = 0x00080000,      // 十，百，千，万，億

  // 最初の文字は FIGURE, KANJI_FIGURE, FIGURE_DIGIT, FIGURE_EXCEPTION
  auto &cp1 = codepoints[start];
  // cp1の文字種をチェック
  if (!cp1.hasClass(charClass_)) {
    return 0;
  }

  size_t longestNumberLength = 0;
  for (LatticePosition pos = 0;
       pos <= MaxNumericLength && start + pos < codepoints.size(); ++pos) {
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

  return 0;
}

} // namespace analysis
} // namespace core
} // namespace jumanpp
